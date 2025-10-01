#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "listener.hpp"
#include <argparse/argparse.hpp>
#include <systemd/sd-daemon.h>
#include <filesystem>

bool load_config(const std::string &config_file, std::string &broker_address, std::string &topic, std::string &db_user, std::string &db_password, std::string &db_name) {
    if (!std::filesystem::exists(config_file)) {
        return false;
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << config_file << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            if (key == "broker_address") {
                broker_address = value;
            } else if (key == "topic") {
                topic = value;
            } else if (key == "db_user") {
                db_user = value;
            } else if (key == "db_password") {
                db_password = value;
            } else if (key == "db_name") {
                db_name = value;
            }
        }
    }

    return true;
}

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("mqtt_listener");

    program.add_argument("-c", "--config")
        .default_value("/etc/mqtt2psql/mqtt2psql.conf")
        .help("Path to the configuration file (default: /etc/mqtt2psql/mqtt2psql.conf)");

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    std::string config_file = program.get<std::string>("--config");
    std::string broker_address = "mqtt://localhost";
    std::string topic = "default/topic";
    std::string db_user = "db_user";
    std::string db_password = "db_password";
    std::string db_name = "measurements_db";

    if (!load_config(config_file, broker_address, topic, db_user, db_password, db_name)) {
        std::cerr << "Using default configuration values." << std::endl;
    }

    std::string db_conn_str = "host=localhost dbname=" + db_name + " user=" + db_user + " password=" + db_password;

    mqtt_listener listener(broker_address, topic, db_conn_str);
    try {
        listener.connect();

        if (sd_notify(0, "READY=1") < 0) {
            std::cerr << "Failed to notify systemd" << std::endl;
        }

        std::cout << "Using configuration file: " << config_file << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();
        listener.disconnect();
    } catch (const mqtt::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}
