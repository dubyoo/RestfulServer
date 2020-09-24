#include "RestfulServer.h"
#include <fstream>
#include <sstream>


#define SSL_CERT_PATH "server.crt"
#define SSL_KEY_PATH "server.key"
#define DEFAULT_USERNAME_PASSWORD "admin"

mg_serve_http_opts RestfulServer::s_server_option;
std::string RestfulServer::mUsername;
std::string RestfulServer::mPassword;

RestfulServer::RestfulServer()
{

}

RestfulServer::~RestfulServer()
{

}


void RestfulServer::Init(const std::string& port)
{
        mUsername = DEFAULT_USERNAME_PASSWORD;
        mPassword = DEFAULT_USERNAME_PASSWORD;

        mPort = port;
        mg_mgr_init(&mManager, NULL);

        struct mg_bind_opts bind_opts;
        memset(&bind_opts, 0, sizeof(bind_opts));
        const char *err;
        bind_opts.ssl_cert = SSL_CERT_PATH;
        bind_opts.ssl_key = SSL_KEY_PATH;
        bind_opts.error_string = &err;

        mg_connection *connection = mg_bind_opt(&mManager, mPort.c_str(), RestfulServer::OnHttpRequestEvent, bind_opts);
        s_server_option.enable_directory_listing = "yes";
        if (NULL == connection)
        {
                printf("Failed to create listener of restful server: %s\n", err);
                return;
        }
        mg_set_protocol_http_websocket(connection);
        printf("Starting restful server on port %s\n", mPort.c_str());
        while (true)
        {
                mg_mgr_poll(&mManager, 60*1000*5);
        }
}


void RestfulServer::OnHttpRequestEvent(mg_connection *connection, int event_type, void *event_data)
{
        if (event_type != MG_EV_HTTP_REQUEST)
        {
                return;
        }

        http_message *http_req = (http_message *)event_data;
        if (CheckUri(http_req, "/api/server.crt"))
        {
                HandleGetCertRequest(connection, http_req);
                return;
        }

        // Check username and password
        char username[32] = "";
        char password[32] = "";
        int authorization = mg_get_http_basic_auth(http_req, username, sizeof(username), password, sizeof(password));
        if (authorization == -1 || !CheckPassword(username, password))
        {
                // response 401 Unauthorized
                mg_http_send_error(connection, 401, "Not Authorized. Incorrect username or password.");
                printf("RESTful API response error code 401\n");
                return;
        }

        // Check "X-Version" in http header
        if (!CheckVersion(connection, http_req))
        {
                // response 406 Not Acceptable and X-Supported-Version 1.0
                const char* reason = "Not Acceptable. Incorrect X-Version.";
                mg_send_head(connection, 406, strlen(reason), "Content-Type: text/plain\r\nConnection: close\r\nX-Supported-Version: 1.0");
                mg_send(connection, reason, strlen(reason));
                connection->flags |= MG_F_SEND_AND_CLOSE;
                printf("RESTful API response error code 406 and X-Version\n");
                return;
        }

        if (CheckUri(http_req, "/api/status"))
        {
                HandleGetStatusRequest(connection, http_req);
        }
        else
        {
                // response 400 Bad Request
                mg_http_send_error(connection, 400, NULL);
                printf("RESTful API response error code 400\n");
        }
}


bool RestfulServer::CheckPassword(const char *username, const char *password)
{
        return strcmp(username, mUsername.c_str()) == 0 && strcmp(password, mPassword.c_str()) == 0;
}

bool RestfulServer::CheckVersion(mg_connection *connection, http_message *http_msg)
{
        mg_str *version = mg_get_http_header(http_msg, "X-Version");
        if (version == NULL)
        {
                return false;
        }

        if (mg_ncasecmp(version->p, "1.0", version->len) != 0)
        {
                return false;
        }

        return true;
}

bool RestfulServer::CheckUri(http_message *http_msg, const char *uri_str)
{
        return mg_vcmp(&http_msg->uri, uri_str) == 0 ? true : false;
}


void RestfulServer::HandleGetCertRequest(mg_connection *connection, http_message *http_msg)
{
        std::ifstream ifs(SSL_CERT_PATH);
        if (!ifs)
        {
                printf("Certificate '%s' does not exist!\n", SSL_CERT_PATH);
                ifs.close();
                // response 500 Internal Server Error
                mg_http_send_error(connection, 500, "Internal Server Error: Cannot find server.crt!");
                printf("RESTful API response error code 500\n");
        }

        std::string cert((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        mg_send(connection, cert.c_str(), cert.length());
        connection->flags |= MG_F_SEND_AND_CLOSE;
        ifs.close();
        printf("RESTful API response server certificate\n");
}

void RestfulServer::HandleGetStatusRequest(mg_connection *connection, http_message *http_msg)
{
        const std::string& response = "{status: OK}";
        mg_printf(connection, "%s", "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(connection, "%s", response.c_str());
        mg_send_http_chunk(connection, "", 0);
        printf("RESTful API response Status\n");
}