from nose.tools import *
from mullet import db
from unittest import TestCase

sess = db.Session("tcp://127.0.0.1:5003")
sql = db.Sql(sess)
disk = db.Disk(sess)
map = db.Mem(sess)
search = db.Search(sess)

def setup():
    sql.query("drop table test")
    sql.query("create table test (id INTEGER PRIMARY KEY, name TEST, age INTEGER)")

def assert_valid(resp):
    print resp
    assert resp
    assert not resp.error()

class Person(db.ResultObject):

    def test(self):
        assert self.name
        assert self.age



class SqlTests(TestCase):

    def test_query(self):
        assert_valid(sql.query("select * from test where name=$name$", vars={"name": "Zed"}))

   
    def test_insert(self):
        res = sql.insert("test", values={"name": "Frank", "age": 70})
        assert_valid(res)
        assert_equal(res.rowcount(), 1)

        res2 = sql.insert("test", values={"name": "Frank", "age": 70})
        assert_valid(res2)
        assert_equal(res.rowcount(), 1)

        assert res._row_id < res2._row_id
        

    def test_where(self): 
        assert_valid(sql.where("test", values={"name": "Frank"}))

    def test_select(self):
        assert_valid(sql.select("test"))

        assert_valid(sql.select("test", what="name"))

        assert_valid(sql.select("test", where="name = $name$", vars={"name": "Frank"}))

        assert_valid(sql.select("test", where="name = $name$", 
                                vars={"name": "Frank"},
                                order="name ASC", limit=2, offset=1))

        res = sql.select("test")
        row = res.first()
        assert row
        assert row.name == "Frank"

    def test_update(self):
        assert_valid(sql.update("test", where="name = $name$ and age = $age",
                         vars={"name": "Frank", "age": 70}, values={"age": 100}))


    def test_delete(self):
        assert_valid(sql.delete("test", where="name = $name$", vars={"name": "Frank"}))


    def test_sql_result(self):
        assert_valid(sql.insert("test", values={"name": "Frank", "age": 70}))
        res = sql.query("select * from test where name='Frank'")
        assert_valid(res)

        for row in res.rows():
            print row

        for row in res:
            print row['age']
            print row['name']

        for row in res.items():
            print row.age
            print row.name

        for person in res.items(Person):
            person.test()
            

class DiskTests(TestCase):

    def test_put(self):
        assert_valid(disk.put("test:1", "this is a test"))

    def test_get(self):
        self.test_put()
        assert_valid(disk.get("test:1"))

    def test_info(self):
        self.test_put()
        assert_valid(disk.info("test:1"))

    def test_cp(self):
        self.test_put()
        assert_valid(disk.cp("test:1", "test:4"))

    def test_mv(self):
        self.test_put()
        assert_valid(disk.mv("test:1", "dead"))

    def test_delete(self):
        self.test_put()
        assert_valid(disk.delete("test:1"))

    def test_putkeep(self):
        disk.delete("pktest")

        status = disk.putkeep("pktest", "test")
        assert_valid(status)
        assert_equal(status.value, True);
        assert_valid(disk.get("pktest"))

        status = disk.putkeep("pktest", "stuff to add")
        assert_valid(status)
        assert_equal(status.value, False);



class MemTests(TestCase):

    def test_put(self):
        assert_valid(map.put("test:1", "this is a test"))

    def test_get(self):
        assert_valid(map.get("test:1"))

    def test_putkeep(self):
        map.delete("test:2")
        assert_valid(map.putkeep("test:2", "this should go in"))
        second = map.putkeep("test:2", "this also should go in")
        assert_valid(second)
        assert not second.value

    def test_putcat(self):
        assert_valid(map.putcat("test:2", " appending to this"))

    def test_store(self):
        disk.delete("test:2")
        assert_valid(map.store("test:2"))
        assert_valid(disk.load("test:2"))
        assert_valid(disk.get("test:2"))


class SearchTests(TestCase):

    def test_put(self):
        assert_valid(search.put(1, "This is a test"))

    def test_get(self):
        self.test_put()
        res = search.get(1)
        assert_valid(res)
        assert_equal(res.value["doc"], "This is a test")

    def test_delete(self):
        self.test_put()
        assert_valid(search.delete(1))
        res = search.get(1)
        assert_equal(res.data["code"], 404)

    def test_query(self):
        self.test_put()
        res = search.query("this || test")
        assert_equal(res.value, {"query":"this || test","ids":[1]})

    def test_find(self):
        self.test_put()
        res = search.find(["this", "test"])
        assert_equal(res.value, {
            'query': 'this test',
            'results': {'this': [1], 'test': [1]}})

    def test_index(self):
        disk.put("toindex", "I'd like to get indexed.")
        res = search.index(3, "toindex")
        assert_valid(res)
        res = search.get(3)
        assert_equal(res.value, {"doc":  "I'd like to get indexed.", "id": 3})

