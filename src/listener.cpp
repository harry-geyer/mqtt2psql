

#include <mqtt/async_client.h>

#include "listener.hpp"


class MqttClientCallback: public virtual mqtt::callback {
public:
    MqttClientCallback(std::function<void(std::string, std::string)> on_message_cb) {
        m_on_message_cb = on_message_cb;
    }

    virtual void message_arrived(mqtt::const_message_ptr msg) override {
        std::cout << "HACK" << std::endl;
        m_on_message_cb(msg->get_topic(), msg->get_payload_str());
    }

private:
    std::function<void(std::string, std::string)>m_on_message_cb;
};


Listener::Listener() {
}

Listener::~Listener() {
    disconnect();
}

void Listener::connect(Listener::ctx_t &ctx) {
    if (is_connected()) {
        return;
    }
    std::ostringstream addr;
    addr << "tcp://" << ctx.address << ":" << ctx.port;
    m_client = std::make_unique<mqtt::async_client>(addr.str());

    mqtt::connect_options conn_opts;
    conn_opts.set_keep_alive_interval(20);
    conn_opts.set_clean_session(true);
    conn_opts.set_user_name(ctx.username);
    conn_opts.set_password(ctx.password);

    MqttClientCallback cb(ctx.on_message_cb);
    m_client->set_callback(cb);

    m_client->connect(conn_opts)->wait();
    m_client->subscribe(ctx.topic, 1)->wait();
}

void Listener::disconnect() {
    if (!is_connected()) {
        return;
    }
}

bool Listener::is_connected() {
    if (!m_client) {
        return false;
    }
    return m_client->is_connected();
}

