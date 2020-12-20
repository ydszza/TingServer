/**

 * @Date    :       2020-12-18
*/
#include "sqlconpool.h"

SqlConPool::SqlConPool() {

}

SqlConPool::~SqlConPool() {
    close_pool();
}

SqlConPool* SqlConPool::instance() {
    static SqlConPool pool;
    return &pool;
}

void SqlConPool::init(const char* host, int port,
                 const char* user, const char* password,
                 const char* db_name, int con_size) {
    assert(host && port > 0 && user && password && db_name && con_size > 0);
    for (int i = 0; i < con_size; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("Mysql init error");
            assert(sql);
        }

        sql = mysql_real_connect(sql, host, user, password, 
                                db_name, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("Mysql real_connect error");
        }   
        conn_queue_.push(sql);
    }
    MAX_CONNECTIONS_ = con_size;
    sem_init(&sem_, 0, MAX_CONNECTIONS_);
}

MYSQL* SqlConPool::get_connection() {
    MYSQL *sql = nullptr;
    if (conn_queue_.empty()) {
        LOG_WARN("Mysqlpool busy!");
    }
    sem_wait(&sem_);
    {
        std::lock_guard<std::mutex> lock(mtx_);
        sql = conn_queue_.front();
        conn_queue_.pop();
    }
    return sql;
}

int SqlConPool::get_free_connection_count() {
    std::lock_guard<std::mutex> lock(mtx_);
    return conn_queue_.size();
}

void SqlConPool::free_connection(MYSQL* connection) {
    assert(connection);
    std::lock_guard<std::mutex> lock(mtx_);
    conn_queue_.push(connection);
    sem_post(&sem_);
}

void SqlConPool::close_pool() {
    std::lock_guard<std::mutex> lock(mtx_);
    while (conn_queue_.size()) {
        auto item = conn_queue_.front();
        conn_queue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}