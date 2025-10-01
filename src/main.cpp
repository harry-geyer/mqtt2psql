#include <iostream>
#include <string>
#include "listener.hpp"
#include <argparse/argparse.hpp>


int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("mqtt_listener");

    program.add_argument("broker_address")
        .help("The address of the MQTT broker");
    program.add_argument("topic")
        .help("The topic to subscribe to");
    program.add_argument("db_connection_string")
        .help("The PostgreSQL connection string");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string broker_address = program.get<std::string>("broker_address");
    std::string topic = program.get<std::string>("topic");
    std::string db_conn_str = program.get<std::string>("db_connection_string");

    mqtt_listener listener(broker_address, topic, db_conn_str);
    try {
        listener.connect();
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        listener.disconnect();
    } catch (const mqtt::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
