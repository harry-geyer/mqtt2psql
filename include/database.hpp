#pragma once

#include <pqxx/pqxx>


class DatabaseConnection {
public:

    struct ctx_t{
        std::string name;
        std::string username;
        std::string password;
        std::string address;
        uint port;
    };

    struct measurement_t{
        std::string sensor;
        std::string name;
        std::chrono::nanoseconds time;
        double value;
    };

    ~DatabaseConnection();
    bool is_connected();
    void connect(ctx_t&);
    void disconnect();
    void create_tables();
    void insert_datapoint(measurement_t&);

private:
    bool write_sql(std::string);

    std::unique_ptr<pqxx::connection> m_db_connection;
};

