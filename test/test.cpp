#include "../code/log/log.h"
#include <assert.h>

int main() {
    Log::instance()->init(1, "./log", ".log", 1024);
    LOG_DEBUG("Starting1");
    LOG_INFO("Starting2");
    LOG_WARN("Starting3");
    LOG_ERROR("Starting4");
    int * error = nullptr;
    //assert(error != nullptr);
}