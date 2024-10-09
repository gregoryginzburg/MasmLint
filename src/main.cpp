#include "add.h"
#include "log.h"

int main()
{
    Log::init();
    int a = add(2, 4);
    LOG_ERROR("Test error {}: {}", "a", a);
    return 0;
}