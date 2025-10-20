#include <iostream>
#include <string>
#include <fstream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <argparse/argparse.hpp>
#ifndef NO_SYSD
#include <systemd/sd-daemon.h>
#endif // NO_SYSD
#include <nlohmann/json.hpp>

#include "database.hpp"
#include "listener.hpp"


using json = nlohmann::json;


class Mqtt2Psql {
public:
    Mqtt2Psql(std::string config_file) {
        std::function<void(std::string, std::string)> listener_on_message_cb =
        [this](std::string topic, std::string payload) {
            this->on_message_cb(topic, payload);
        };

        DatabaseConnection::ctx_t db_ctx = {
            .name = "measurements_db",
            .username = "db_user",
            .password = "db_password",
            .address = "localhost",
            .port = 5432,
        };

        Listener::ctx_t listener_ctx = {
            .address = "localhost",
            .port = 1883,
            .username = "mqtt_user",
            .password = "mqtt_password",
            .topic = "measurements/#",
            .on_message_cb = listener_on_message_cb,
        };

        if (!load_config(config_file, db_ctx, listener_ctx)) {
            std::cerr << "Using default configuration values." << std::endl;
        }

        m_db.connect(db_ctx);
        m_db.create_tables();

        m_listener.connect(listener_ctx);
    }

    void run() {
        std::cout << "Running... press Ctrl+C to exit." << std::endl;
        std::signal(SIGINT, signal_handler);
        while (!m_stop_flag.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::cout << "\rShutting down..." << std::endl;
    }

private:
    DatabaseConnection m_db;
    Listener m_listener;
    static inline std::atomic<bool> m_stop_flag = false;

    bool load_config(const std::string &config_file, DatabaseConnection::ctx_t &db_ctx, Listener::ctx_t &listener_ctx) {
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
                            listener_ctx.address = value;
                        } else if (key == "port") {
                            listener_ctx.port = std::stoul(value);
                        } else if (key == "topic") {
                            listener_ctx.topic = value;
                        } else if (key == "username") {
                            listener_ctx.username = value;
                        } else if (key == "password") {
                            listener_ctx.password = value;
                        }
                    } else if (current_section == "Database") {
                        if (key == "db_name") {
                            db_ctx.name = value;
                        } else if (key == "db_password") {
                            db_ctx.password = value;
                        } else if (key == "db_name") {
                            db_ctx.username = value;
                        } else if (key == "db_host") {
                            db_ctx.address = value;
                        } else if (key == "db_port") {
                            db_ctx.port = std::stoul(value);
                        }
                    }
                }
            }
        }

        return true;
    }

    void on_message_cb(std::string topic, std::string payload) {
        std::cout << "Topic: " << topic << std::endl;
        std::cout << "Payload: " << payload << std::endl;

        std::chrono::system_clock::duration now =
            std::chrono::system_clock::now().time_since_epoch();

        std::chrono::nanoseconds now_ns =
            std::chrono::duration_cast<std::chrono::nanoseconds>(now);

        std::vector<DatabaseConnection::measurement_t> measurements;
        try {
            json j = json::parse(payload);
            std::string sensor_name = j.value("sensor", "");
            if (sensor_name.empty()) {
                std::cerr << "payload missing sensor name" << std::endl;
                return;
            }
            if (!j.contains("measurements") || !j["measurements"].is_array()) {
                std::cerr << "payload missing measurements array" << std::endl;
                return;
            }
            const json& measurement_array = j["measurements"];
            for (std::size_t i = 0; i < measurement_array.size(); ++i) {
                const json& jm = measurement_array[i];

                std::string name = jm.value("name", "");
                if (name.empty()) {
                    std::cerr << "payload missing measurement name" << std::endl;
                    continue;
                }

                double value = jm.value("value", 0.0);

                DatabaseConnection::measurement_t m;
                m.sensor = sensor_name;
                m.name = name;
                m.time = now_ns;
                m.value = value;

                measurements.push_back(m);
            }
        } catch (const json::parse_error& e) {
            std::cerr << "JSON parse error: " << e.what() << std::endl;
            return;
        } catch (const json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << std::endl;
            return;
        }
        for (std::size_t i = 0; i < measurements.size(); ++i) {
            DatabaseConnection::measurement_t& m = measurements[i];
            m_db.insert_datapoint(m);
        }
    }

    static void signal_handler(int signal) {
        if (signal == SIGINT) {
            m_stop_flag.store(true);
        }
    }
};


int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("mqtt2psql", VERSION_NUMBER);

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
    Mqtt2Psql mqtt2psql(config_file);

#ifndef NO_SYSD
    if (sd_notify(0, "READY=1") < 0) {
        std::cerr << "Failed to notify systemd" << std::endl;
    }
#endif // NO_SYSD

    mqtt2psql.run();

    return 0;
}
