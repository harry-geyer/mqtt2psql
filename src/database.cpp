#include <string>
#include <sstream>
#include <iostream>

#include "database.hpp"


#define DATABASE_DB_VERSION         "1.0.0"


DatabaseConnection::~DatabaseConnection() {
    disconnect();
}

bool DatabaseConnection::is_connected() {
    if (!m_db_connection) {
        return false;
    }
    return m_db_connection->is_open();
}

void DatabaseConnection::connect(ctx_t &database_ctx) {
    if (is_connected()) {
        /* already connected */
        return;
    }
    std::ostringstream conn_str;
    conn_str << "dbname = " << database_ctx.name
        << " user = " << database_ctx.username
        << " password = " << database_ctx.password
        << " hostaddr = " << database_ctx.address
        << " port = " << database_ctx.port;
    m_db_connection = std::make_unique<pqxx::connection>(conn_str.str());
}

void DatabaseConnection::disconnect() {
    if (!is_connected()) {
        /* already disconnected */
        return;
    }
}

void DatabaseConnection::create_tables() {
    if (!is_connected()) {
        throw std::runtime_error("database not connected");
    }
    pqxx::work cursor(*m_db_connection);
    std::ostringstream sql;
    sql << "CREATE TABLE IF NOT EXISTS metadata("
        "id SERIAL PRIMARY KEY NOT NULL,"
        "name TEXT NOT NULL,"
        "value TEXT NOT NULL"
        ");";
    cursor.exec(sql.str());
    sql.str("");
    sql.clear();

    sql << "SELECT value FROM metadata WHERE name = 'version';";
    pqxx::result res(cursor.exec(sql.str()));
    sql.str("");
    sql.clear();
    int res_size = res.size();
    if (res_size > 1) {
        throw std::runtime_error("too many version numbers in db");
    } else if (res_size == 0) {
        /* new database */
        std::cerr << "Writing new database" << std::endl;
        sql << "INSERT INTO metadata(name, value) "
            "VALUES('version', '" << DATABASE_DB_VERSION << "');";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();

        sql << "CREATE TABLE IF NOT EXISTS sensors("
            "id SERIAL PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL UNIQUE,"
            "about TEXT"
            ");";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();

        sql << "CREATE TABLE IF NOT EXISTS measurement_types("
            "id SERIAL PRIMARY KEY NOT NULL,"
            "name TEXT NOT NULL UNIQUE"
            ");";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();
        sql << "CREATE TABLE IF NOT EXISTS measurements("
            "id SERIAL PRIMARY KEY NOT NULL,"
            "time BIGINT NOT NULL,"
            "value DOUBLE PRECISION NOT NULL,"
            "sensor_id INT NOT NULL,"
            "type_id INT NOT NULL,"
            "CONSTRAINT fk_type"
            "  FOREIGN KEY (sensor_id) REFERENCES sensors(id),"
            "  FOREIGN KEY (type_id) REFERENCES measurement_types(id)"
            ");";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();
        cursor.commit();
    } else if (res_size < 0) {
        throw std::runtime_error("Negative rows returned");
    } else {
        pqxx::result::const_iterator c = res.begin();
        if (c[0].as<std::string>() != DATABASE_DB_VERSION) {
            throw std::runtime_error("need to migrate database");
        }
    }
}

void DatabaseConnection::insert_datapoint(measurement_t &m) {
    pqxx::work cursor(*m_db_connection);
    std::ostringstream qsql;
    std::ostringstream sql;

    qsql << "SELECT id FROM sensors WHERE name = '" << m.sensor << "';";
    pqxx::result res(cursor.exec(qsql.str()));
    int res_size = res.size();
    if (res_size == 0) {
        sql << "INSERT INTO sensors(name) VALUES ('" << m.sensor << "');";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();
        res = pqxx::result(cursor.exec(qsql.str()));
    } else if (res_size > 1) {
        std::cerr << "multiple sensors with this name: " << m.sensor << std::endl;
        return;
    } else if (res_size < 0) {
        std::cerr << "returned negative rows" << std::endl;
        return;
    }
    qsql.str("");
    qsql.clear();
    pqxx::result::const_iterator pos = res.begin();
    int sensor_id = pos[0].as<int>();

    qsql << "SELECT id FROM measurement_types WHERE name = '" << m.name << "';";
    res = pqxx::result(cursor.exec(qsql.str()));
    res_size = res.size();
    if (res_size == 0) {
        sql << "INSERT INTO measurement_types(name) VALUES ('" << m.name << "');";
        cursor.exec(sql.str());
        sql.str("");
        sql.clear();
        res = pqxx::result(cursor.exec(qsql.str()));
    } else if (res_size > 1) {
        std::cerr << "multiple measurements with this name: " << m.name << std::endl;
        return;
    } else if (res_size < 0) {
        std::cerr << "returned negative rows" << std::endl;
        return;
    }
    pos = res.begin();
    int measurement_id = pos[0].as<int>();

    sql << "INSERT INTO measurements(sensor_id, type_id, time, value)"
        "VALUES(" << sensor_id << "," << measurement_id << "," << m.time.count() << "," << m.value << ");";
    cursor.exec(sql.str());
    cursor.commit();
}

bool DatabaseConnection::write_sql(std::string sql)
{
    if (!is_connected()) {
        return false;
    }
    try {
        pqxx::work cursor(*m_db_connection);
        cursor.exec(sql);
        cursor.commit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}
