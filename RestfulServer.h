#ifndef __RESTFUL_SERVER_H__
#define __RESTFUL_SERVER_H__

#include "mongoose.h"
#include <string>


class RestfulServer
{
public:
        RestfulServer();
        virtual ~RestfulServer();
        void Init(const std::string& port);
        void Loop();

        static mg_serve_http_opts s_server_option;

private:
        static void OnHttpRequestEvent(mg_connection *connection, int event_type, void *event_data);
        static bool CheckPassword(const char *username, const char *password);
        static bool CheckVersion(mg_connection *connection, http_message *http_msg);
        static bool CheckUri(http_message *http_msg, const char *uri_str);
        static void HandleGetCertRequest(mg_connection *connection, http_message *http_msg);
        static void HandleGetStatusRequest(mg_connection *connection, http_message *http_msg);

        mg_mgr mManager;
        std::string mPort;
        static std::string mUsername;
        static std::string mPassword;
};

#endif