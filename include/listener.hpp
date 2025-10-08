#pragma once

#include <mqtt/async_client.h>


class Listener {
public:
    struct ctx_t {
        std::string address;
        uint port;
        std::string username;
        std::string password;
        std::string topic;
        std::function<void(std::string, std::string)> on_message_cb;
    };
    Listener();
    ~Listener();

    void connect(ctx_t&);
    void disconnect();
    bool is_connected();

private:
    std::unique_ptr<mqtt::async_client> m_client;
};
