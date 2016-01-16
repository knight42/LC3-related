#include "command.h"


int main(int argc, char *argv[])
{
    lc3_init_machine(argv[1]);
    lc3_run();
    return 0;
}

