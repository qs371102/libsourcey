#ifndef SCY_HTTP_Tests_H
#define SCY_HTTP_Tests_H


#include "scy/interface.h"
#include "scy/base.h"
#include "scy/crypto/hash.h"
#include "scy/filesystem.h"
#include "scy/http/client.h"
#include "scy/http/connection.h"
#include "scy/http/form.h"
#include "scy/http/packetizers.h"
#include "scy/http/server.h"
#include "scy/http/url.h"
#include "scy/http/util.h"
#include "scy/http/websocket.h"
#include "scy/idler.h"
#include "scy/net/sslcontext.h"
#include "scy/net/sslmanager.h"
#include "scy/test.h"
#include "scy/timer.h"

#include "../samples/httpechoserver/httpechoserver.h"


using std::cout;
using std::cerr;
using std::endl;
using scy::test::Test;


#define TEST_HTTP_PORT 1337
#define TEST_HTTPS_PORT 1338


namespace scy {


//
/// HTTP Client Tests
//

/// Initializes a polymorphic HTTP client connection for
/// testing callbacks, and also optionally raises the server.
struct HTTPEchoTest
{
    http::Server server;
    http::Client client;
    http::ClientConnection::Ptr conn;
    int numSuccess;
    int numWanted;

    HTTPEchoTest(int numWanted = 1)
        : server(net::Address("0.0.0.0", TEST_HTTP_PORT))
        , numSuccess(0)
        , numWanted(numWanted)
    {
    }

    void raiseServer() 
    { 
        server.start();
        server.Connection += [](http::ServerConnection::Ptr conn) {
            conn->Payload += [](http::ServerConnection& conn, const MutableBuffer& buffer) {
                conn.send(bufferCast<const char*>(buffer), buffer.size());
            };
        };
    }

    http::ClientConnection::Ptr createConnection(const std::string& protocol, const std::string& query)
    {
        std::ostringstream url;
        url << protocol << "://127.0.0.1:" << TEST_HTTP_PORT << query << endl;
        conn = client.createConnection(url.str());
        conn->Connect += slot(this, &HTTPEchoTest::onConnect);
        conn->Headers += slot(this, &HTTPEchoTest::onHeaders);
        conn->Payload += slot(this, &HTTPEchoTest::onPayload);
        conn->Complete += slot(this, &HTTPEchoTest::onComplete);
        conn->Close += slot(this, &HTTPEchoTest::onClose);
        return conn;
    }

    void start() 
    { 
        conn->send("PING", 4); 
    }

    void shutdown()
    {
        // Stop the client and server to release the loop
        server.shutdown();
        client.shutdown();

        expect(numSuccess == numWanted);
    }

    void onConnect()
    {
        DebugL << "On connect" << endl;
    }

    void onHeaders(http::Response& res)
    {
        DebugL << "On headers" << endl;
    }

    void onComplete(const http::Response& res)
    {
        std::ostringstream os;
        res.write(os);
        DebugL << "Response complete: " << os.str() << endl;
    }

    void onPayload(const MutableBuffer& buffer)
    {
        std::string data(bufferCast<const char*>(buffer), buffer.size());
        DebugL << "On payload: " << buffer.size() << ": " << data << endl;

        if (data == "PING")
            numSuccess++;

        if (numSuccess == numWanted) {
            shutdown();
        } else {
            conn->send("PING", 4);
        }
    }

    void onClose(http::Connection&)
    {
        DebugL << "Connection closed" << endl;
        shutdown();
    }
};


} // namespace scy


#endif // SCY_HTTP_Tests_H


/// @\}
