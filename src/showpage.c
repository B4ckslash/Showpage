#include <stdio.h>
#include <stdlib.h>
#include "ShowPageConf.h"

int main(int argc, char *argv[])
{
    fprintf(stdout, "%s Version %d.%d\n",
            argv[0],
            ShowPage_VERSION_MAJOR,
            ShowPage_VERSION_MINOR);
    return 0;
}
