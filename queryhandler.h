#ifndef __queryhandler__
#define __queryhandler__

#include "mulletdb.h"
#include "storage.h"
#include <zmq.hpp>

#define DEFAULT_PORT "tcp://127.0.0.1:5003"
#define DEFAULT_SUBSCRIBE "tcp://127.0.0.1:5004"
#define DEFAULT_PUBLISH "tcp://127.0.0.1:5005"

class QueryHandler : public thread
{

    public:

    string inaddr;
    string subaddr;
    string pubaddr;
    Storage store;
    zmq::context_t ctx;
    zmq::socket_t in_socket;
    zmq::socket_t proc_socket;
    zmq::socket_t sub_socket;
    zmq::socket_t pub_socket;


    QueryHandler (const value &config) : thread ("QueryHandler"),
            ctx(2,2,ZMQ_POLL), in_socket(ctx, ZMQ_REP), proc_socket(ctx, ZMQ_SUB),
            sub_socket(ctx, ZMQ_SUB), pub_socket(ctx, ZMQ_PUB),
            store(config)
    {
         inaddr = config.exists("listen") ? config["listen"] : DEFAULT_PORT;
         subaddr = config.exists("subscribe") ? config["subscribe"] : DEFAULT_SUBSCRIBE;
         pubaddr = config.exists("publish") ? config["publish"] : DEFAULT_PUBLISH;

    }

    ~QueryHandler (void) {}

    value parse_request(string qstr);
   
    void perform_request(zmq::socket_t &in, zmq::socket_t &out);

    void run(void);

    void die(void);
};

#endif
