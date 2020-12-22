/**
 * @Date    :       2020-12-18
*/

#include "server/webserver.h"

int main(int argc, char** argv) {
    //实例化一个web服务
    WebServer server(3880, 3, 60000, false, 4, true, 0, 1024);
    server.start();
    return 0;
}