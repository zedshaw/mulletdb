import zmq
import simplejson as json

CTX = zmq.Context(1)

class ResultObject(object):

    def __init__(self, data):
        for x,v in data.items():
            setattr(self, x, v)

    def __repr__(self):
        return repr(self.__dict__)


class Result(object):

    def __init__(self, query, resp):
        self.query = query
        self.data = json.loads(resp)

        self.code = self.data["code"]
        self.status = self.data["status"]
        self.message = self.data["message"]
        self.value = self.data["value"]
        self.target = self.data["target"]
        self.command = self.data["command"]

    def as_json(self):
        return json.loads(self.value)

    def error(self):
        return self.code != 200 and self.data


class ResultSet(Result):

    def __init__(self, query, resp):
        super(ResultSet, self).__init__(query, resp)

        if not self.error():
            self.columns = self.value["columns"]
            self.row_cache = []

            if '_row_id' in self.columns:
                assert len(self.columns) == 1, "Got wrong number of columns on insert."
                self._row_id = int(self.value["data"][0][0])

    def rowcount(self):
        return len(self.value["data"])


    def __repr__(self):
        return repr(self.data)

    def __iter__(self):
        if self.row_cache:
            for row in self.row_cache:
                yield row
        else:
            for row in self.value["data"]:
                data = dict(zip(self.columns, row))
                self.row_cache.append(data)
                yield data

    def rows(self):
        assert self.target == "sql", "only for sql results"
        return self.value["data"]

    def items(self, klass=ResultObject):
        assert self.target == "sql", "only for sql results"

        for data in self:
            yield klass(data)

    def first(self, klass=ResultObject):
        assert self.target == "sql", "only for sql results"
        if not self.value["data"]: 
            return None
        else:
            return klass(dict(zip(self.columns, self.value["data"][0])))



class Session(object):

    def __init__(self, addr, publish=False):
        self.addr = addr
        self.publish = publish
        self.socket_type = zmq.PUB if publish else zmq.REQ
        self.Q = zmq.Socket(CTX, self.socket_type)
        self.Q.connect(addr)

    def send(self, query):
        self.Q.send(query + "\0");
        return not self.publish and self.Q.recv()

    def request(self, query, klass=Result):
        return not self.publish and klass(query, self.send(query))



class Mem(object):

    def __init__(self, session):
        self.sess = session

    def request(self, *data):
        return self.sess.request("mem." + " ".join(data))

    def put(self, key, value):
        return self.request('put', key, value)

    def get(self, key):
        return self.request('get', key)

    def putkeep(self, key, value):
        return self.request('putkeep', key, value)

    def putcat(self, key, value):
        return self.request('putcat', key, value)

    def delete(self, key):
        return self.request('del', key)

    def store(self, key):
        return self.request('store', key)


class Disk(object):

    def __init__(self, session):
        self.sess = session

    def request(self, *data):
        return self.sess.request("disk." + " ".join(data))

    def put(self, key, value):
        return self.request('put', key, value)

    def putkeep(self, key, value):
        return self.request('putkeep', key, value)

    def get(self, key):
        return self.request('get', key)

    def delete(self, key):
        return self.request('delete', key)

    def info(self, key):
        return self.request('info', key)

    def cp(self, from_key, to_key):
        return self.request('cp', from_key, to_key)

    def mv(self, from_key, to_key):
        return self.request('mv', from_key, to_key)

    def load(self, key):
        return self.request('load', key)


class Sql(object):

    def __init__(self, session):
        self.sess = session

    def request(self, action, data):
        return self.sess.request("sql." + action + " " + json.dumps(data), klass=ResultSet)

    def query(self, sql, vars={}):
        req = {"sql": sql, "vars": vars}
        return self.request("query", req)

    def select(self, table, **kw):
        """
            {"table": "", "vars": {}, "what": "*", "where":"", "order":"", 
            "group": "", "limit": "", "offset": "", "having": ""}
        """
        kw["table"] = table
        return self.request("select", kw)

    def where(self, table, values={}):
        req = {"table": table, "values": values}
        return self.request("where", req)

    def insert(self, table, values={}):
        req = {"table": table, "values": values}
        return self.request("insert", req)

    def update(self, table, **kw):
        """ {"table": "", "where": "", "vars": {}, "values": {}} """
        kw["table"] = table
        return self.request("update", kw)
        
    def delete(self, table, **kw):
        """ * delete {"table": "", "where": "", "using": "", vars={}} """
        kw["table"] = table
        return self.request("delete", kw)


class Search(object):

    def __init__(self, session):
        self.sess = session

    def request(self, action, *data):
        return self.sess.request("doc." + action + " " + " ".join(data))

    def get(self, id):
        return self.request("get", str(id))

    def put(self, id, value):
        return self.request("put", str(id), value)

    def delete(self, id):
        return self.request("del", str(id))

    def query(self, expr):
        return self.request("query", expr)

    def find(self, words):
        return self.request("find", *words)

    def index(self, id, key):
        return self.request("index", str(id), key)

