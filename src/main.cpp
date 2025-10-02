#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include "listener.hpp"
#include <argparse/argparse.hpp>
#include <systemd/sd-daemon.h>
#include <filesystem>
#include <csignal>

bool load_config(const std::string &config_file, std::string &broker_address, std::string &topic, std::string &db_user, std::string &db_password, std::string &db_name, std::string &db_host) {
    if (!std::filesystem::exists(config_file)) {
        return false;
    }

    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << config_file << std::endl;
        return false;
    }

    std::string line;
    std::string current_section;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.starts_with('[') && line.ends_with(']')) {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }

        if (!current_section.empty() && line.find('=') != std::string::npos) {
            std::istringstream iss(line);
            std::string key, value;
            if (std::getline(iss, key, '=') && std::getline(iss, value)) {
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (current_section == "MQTT") {
                    if (key == "broker_address") {
                        broker_address = value;
                    } else if (key == "topic") {
                        topic = value;
                    }
                } else if (current_section == "Database") {
                    if (key == "db_user") {
                        db_user = value;
                    } else if (key == "db_password") {
                        db_password = value;
                    } else if (key == "db_name") {
                        db_name = value;
                    } else if (key == "db_host") {
                        db_host = value;
                    }
                }
            }
        }
    }

    return true;
}

void signal_handler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received. Exiting..." << std::endl;
    exit(signum);
}

int main(int argc, char *argv[]) {
    // Register signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

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
    std::string db_host = "localhost";

    if (!load_config(config_file, broker_address, topic, db_user, db_password, db_name, db_host)) {
        std::cerr << "Using default configuration values." << std::endl;
    }

    std::string db_conn_str = "host=" + db_host + " dbname=" + db_name + " user=" + db_user + " password=" + db_password;

    std::unique_ptr<mqtt_listener> listener = std::make_unique<mqtt_listener>(broker_address, topic, db_conn_str);

    try {
        listener->connect();

        if (sd_notify(0, "READY=1") < 0) {
            std::cerr << "Failed to notify systemd" << std::endl;
        }

        std::cout << "Using configuration file: " << config_file << std::endl;
        std::cout << "Press Enter to exit..." << std::endl;
        std::cin.get();

        listener->disconnect();
    } catch (const mqtt::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return 1;
    }

    return 0;
}

