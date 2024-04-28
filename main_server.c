#include <stdio.h>
#include "hdr/configurator.h"
#include "hdr/network.h"

int main(void)
{
    read_config();

    main_server();

    return 0;
}
