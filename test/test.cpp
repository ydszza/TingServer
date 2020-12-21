#include "../code/log/log.h"

int main() {
    Log::instance()->init(1, "./log", ".log", 1024);
    for (int i = 0; i < 4; ++i) {
        LOG_BASE(i, "%d", i);
    }
}