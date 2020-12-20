/**
 * @Date    :       2020-12-18
*/

#include "webserver.h"

int main(int argc, char** argv) {
    //实例化一个web服务
    WebServer server(80, 3, 60000, false, 4, true, 1, 1024);
    server.start();
    return 0;
}