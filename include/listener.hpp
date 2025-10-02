#pragma once

#include <string>
#include <mqtt/async_client.h>
#include <pqxx/pqxx>
#include <memory>

class mqtt_listener : public virtual mqtt::callback {
public:
    mqtt_listener(const std::string &address, const std::string &topic, const std::string &db_conn_str);
    ~mqtt_listener();
    void connect();
    void disconnect();
    void on_message(const mqtt::const_message_ptr &msg);

private:
    mqtt::async_client client;
    std::string topic;
    std::string db_conn_str;
    std::unique_ptr<pqxx::connection> db_connection;

    void connect_to_postgresql();
    void send_to_postgresql(const std::string &json_string);
};

