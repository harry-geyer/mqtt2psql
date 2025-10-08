#pragma once

#include <mqtt/async_client.h>


class ListenerMqttClientCallback: public virtual mqtt::callback {
public:
    ListenerMqttClientCallback(std::function<void(std::string, std::string)> on_message_cb) {
        m_on_message_cb = on_message_cb;
    }

    virtual void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "MQTT received message" << std::endl;
        m_on_message_cb(msg->get_topic(), msg->get_payload_str());
    }

private:
    std::function<void(std::string, std::string)>m_on_message_cb;
};


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
    std::unique_ptr<ListenerMqttClientCallback> m_mqtt_client_callback;
};
