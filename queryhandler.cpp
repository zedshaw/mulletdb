#include "queryhandler.h"

#include <errno.h>

void our_free(void *data, void *hint)
{
    free(data);
}


value QueryHandler::parse_request(string qstr)
{
    string target = qstr.cutat('.');
    string cmd = qstr.cutat(' ');
    value result;

    if(target == "mem") {
        result = store.mem(cmd, qstr);
    } else if(target == "sql") {
        result = store.sql(cmd, qstr);
    } else if(target == "disk") {
        result = store.disk(cmd, qstr);
    } else if(target == "doc") {
        result = store.doc(cmd, qstr);
    } else {
        result = store.response("error", INVALID_REQUEST, "invalid command target", target);
    }

    result["target"] = target;
    result["command"] = cmd;

    return result;
}


void QueryHandler::perform_request(zmq::socket_t &in, zmq::socket_t &out)
{
    zmq::message_t query;
    in.recv(&query);

    string qstr = (const char *)query.data();

    value results = parse_request(qstr);

    string json = results.tojson();
    zmq::message_t payload(strdup(json.cval()), json.strlen(), our_free, NULL);

    out.send(payload, 0);
}


void QueryHandler::run(void)
{
    zmq::pollitem_t items[3];

    log::write(log::info, "query", "receiving requests on %s" %format(inaddr));
    in_socket.bind(inaddr);

    proc_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    proc_socket.bind("inproc://control");

    
    log::write(log::info, "query", "receiving async requests on %s" %format(subaddr));
    sub_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    sub_socket.bind(subaddr);

    log::write(log::info, "query", "publishing async responses on %s" %format(pubaddr));
    pub_socket.bind(pubaddr);


    items[0].socket = in_socket;
    items[0].events = ZMQ_POLLIN;
    items[1].socket = proc_socket;
    items[1].events = ZMQ_POLLIN;
    items[2].socket = sub_socket;
    items[2].events = ZMQ_POLLIN;


    while(true)
    {
        int res = zmq::poll(items, 3, -1);

        if(items[0].revents == ZMQ_POLLIN) {
            perform_request(in_socket, in_socket);
        } 
        
        if (items[2].revents == ZMQ_POLLIN) { 
            perform_request(sub_socket, pub_socket);
        } 

        if(items[1].revents == ZMQ_POLLIN) {
            zmq::message_t query;
            proc_socket.recv(&query);
            break;
        }
    }

    log::write(log::info, "query", "queryhandler thread exiting: errno %d: %s" %format(errno, strerror(errno)));
}


void QueryHandler::die(void) {
    zmq::socket_t proc(ctx, ZMQ_PUB);
    proc.connect("inproc://control");

    zmq::message_t msg(strdup("die"), 3, our_free, NULL);
    proc.send(msg, 0);
}
