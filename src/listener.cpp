

#include <mqtt/async_client.h>

#include "listener.hpp"


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

    m_mqtt_client_callback = std::make_unique<ListenerMqttClientCallback>(ctx.on_message_cb);
    m_client->set_callback(*m_mqtt_client_callback);

    m_client->connect(conn_opts)->wait();
    std::cout << "MQTT connected" << std::endl;
    m_client->subscribe(ctx.topic, 1)->wait();
    std::cout << "MQTT subscribed" << std::endl;
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

