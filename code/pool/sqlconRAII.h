/**

 * @Date    :       2020-12-18
*/

#ifndef __SQLCONRAII_H_
#define __SQLCONRAII_H_

#include "sqlconpool.h"

class SqlConRAII {
public:
    SqlConRAII(MYSQL** sql, SqlConPool* conpool) {
        assert(conpool);
        *sql = conpool->get_connection();
        sql_ = *sql;
        conpool_ = conpool;
    }

    ~SqlConRAII() { 
        conpool_->free_connection(sql_);
    }

private:
    MYSQL* sql_;
    SqlConPool* conpool_;
};


#endif // !__SQLCONRAII_H_