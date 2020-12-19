/**
 * @Author:         yds
 * @Date    :       2020-12-18
*/

#include "tcpserver.h"

int main(int argc, char** argv) {
    TcpServer server(
        1234, 3, 60000, false, 
        3306, "root", "root", "server", 
        12, 4, true, 1, 1024
    );
    server.start();
    return 0;
}