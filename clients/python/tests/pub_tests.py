from nose.tools import *
from mullet import db
from unittest import TestCase

sess = db.Session("tcp://127.0.0.1:5004", publish=True)
sql = db.Sql(sess)
disk = db.Disk(sess)
map = db.Mem(sess)

def setup():
    sql.query("drop table test")
    sql.query("create table test (id INTEGER PRIMARY KEY, name TEST, age INTEGER)")

def assert_valid(resp):
    assert resp == False


class SqlTests(TestCase):

    def test_query(self):
        assert_valid(sql.query("select * from test where name=$name$", vars={"name": "Zed"}))

    def test_insert(self):
        res = sql.insert("test", values={"name": "Frank", "age": 70})
        assert_valid(res)

        res2 = sql.insert("test", values={"name": "Frank", "age": 70})
        assert_valid(res2)

    def test_where(self): 
        assert_valid(sql.where("test", values={"name": "Frank"}))

    def test_select(self):
        assert_valid(sql.select("test"))

        assert_valid(sql.select("test", what="name"))

        assert_valid(sql.select("test", where="name = $name$", vars={"name": "Frank"}))

        assert_valid(sql.select("test", where="name = $name$", 
                                vars={"name": "Frank"},
                                order="name ASC", limit=2, offset=1))

    def test_update(self):
        assert_valid(sql.update("test", where="name = $name$ and age = $age",
                         vars={"name": "Frank", "age": 70}, values={"age": 100}))


    def test_delete(self):
        assert_valid(sql.delete("test", where="name = $name$", vars={"name": "Frank"}))


    def test_sql_result(self):
        assert_valid(sql.insert("test", values={"name": "Frank", "age": 70}))
        res = sql.query("select * from test where name='Frank'")
        assert_valid(res)


class DiskTests(TestCase):

    def test_put(self):
        assert_valid(disk.put("test:1", "this is a test"))

    def test_get(self):
        assert_valid(disk.get("test:1"))

    def test_info(self):
        assert_valid(disk.info("test:1"))

    def test_cp(self):
        assert_valid(disk.cp("test:1", "test:4"))

    def test_mv(self):
        assert_valid(disk.mv("test:4", "dead"))

    def test_delete(self):
        assert_valid(disk.delete("dead"))

    def test_putkeep(self):
        disk.delete("pktest")

        status = disk.putkeep("pktest", "test")
        assert_valid(status)
        assert_valid(disk.get("pktest"))

        status = disk.putkeep("pktest", "stuff to add")
        assert_valid(status)



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

    def test_putcat(self):
        assert_valid(map.putcat("test:2", " appending to this"))

    def test_store(self):
        disk.delete("test:2")
        assert_valid(map.store("test:2"))
        assert_valid(disk.load("test:2"))
        assert_valid(disk.get("test:2"))


