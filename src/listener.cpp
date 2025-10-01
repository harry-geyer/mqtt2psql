#include "listener.hpp"
#include <iostream>
#include <cjson/cJSON.h>
#include <pqxx/pqxx>


mqtt_listener::mqtt_listener(const std::string &address, const std::string &topic, const std::string &db_conn_str)
    : client(address, ""), topic(topic), db_conn_str(db_conn_str), db_connection(nullptr) {
    client.set_callback(*this);
    connect_to_postgresql();
}

void mqtt_listener::connect() {
    mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);
    std::cout << "Connecting to the MQTT broker..." << std::endl;
    client.connect(conn_opts)->wait();
    std::cout << "Connected!" << std::endl;

    client.subscribe(topic, 1);
    std::cout << "Subscribed to topic: " << topic << std::endl;
}

void mqtt_listener::disconnect() {
    std::cout << "Disconnecting..." << std::endl;
    client.disconnect()->wait();
    std::cout << "Disconnected!" << std::endl;

    if (db_connection) {
        delete db_connection;
        db_connection = nullptr;
    }
}

void mqtt_listener::connect_to_postgresql() {
    try {
        db_connection = new pqxx::connection(db_conn_str);
        std::cout << "Connected to PostgreSQL!" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Error connecting to PostgreSQL: " << e.what() << std::endl;
    }
}

void mqtt_listener::on_message(const mqtt::const_message_ptr &msg) {
    std::cout << "Message received: " << msg->get_payload() << std::endl;

    const std::string json_payload = msg->get_payload();
    send_to_postgresql(json_payload);
}

void mqtt_listener::send_to_postgresql(const std::string &json_string) {
    cJSON *json_data = cJSON_Parse(json_string.c_str());
    if (json_data == nullptr) {
        std::cerr << "Error parsing JSON" << std::endl;
        return;
    }

    const cJSON *key = cJSON_GetObjectItemCaseSensitive(json_data, "key");
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(json_data, "value");

    if (cJSON_IsString(key) && (key->valuestring != nullptr) &&
        cJSON_IsString(value) && (value->valuestring != nullptr)) {

        try {
            pqxx::work W(*db_connection);
            std::string query = "INSERT INTO your_table (key, value) VALUES (" + W.quote(key->valuestring) + ", " + W.quote(value->valuestring) + ");";
            W.exec(query);
            W.commit();
            std::cout << "Data inserted into PostgreSQL: " << key->valuestring << ", " << value->valuestring << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Error inserting data into PostgreSQL: " << e.what() << std::endl;
        }
    }

    cJSON_Delete(json_data);
}
