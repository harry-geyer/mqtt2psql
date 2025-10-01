#pragma once

#include <string>
#include <mqtt/async_client.h>
#include <pqxx/pqxx>


class mqtt_listener : public virtual mqtt::callback {
public:
    mqtt_listener(const std::string &address, const std::string &topic, const std::string &db_conn_str);
    void connect();
    void disconnect();
    void on_message(const mqtt::const_message_ptr &msg);

private:
    mqtt::async_client client;
    std::string topic;
    std::string db_conn_str;
    pqxx::connection *db_connection;

    void connect_to_postgresql();
    void send_to_postgresql(const std::string &json_string);
};
