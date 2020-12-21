/**

 * @Date    :       2020-12-18
*/
#ifndef __SQLCONPOOL_H_
#define __SQLCONPOOL_H_

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <assert.h>
#include <semaphore.h>
#include "../log/log.h"

class SqlConPool {
public:
    static SqlConPool * instance();

    MYSQL* get_connection();
    void free_connection(MYSQL* connection);
    int get_free_connection_count();

    void init(const char* host, int port,
              const char* user, const char* password,
              const char* db_name, int con_size);
    void close_pool();

private:    
    SqlConPool();
    ~SqlConPool();

private:
    int MAX_CONNECTIONS_;

    std::queue<MYSQL *> conn_queue_;
    std::mutex mtx_;
    sem_t sem_;
};

#endif // !__SQLCONPOOL_H_