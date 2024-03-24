#include "hdr/configurator.h"
#include "hdr/network.h"
#include "hdr/graphics.h"

#include <stdio.h>
#include <locale.h>

int main(void)
{
    int ret = 1;
    setlocale (LC_ALL, "");

    read_config();
    init_graphics();

    while(ret > 0)
    {
        switch (ret = menu_wnd())
        {
            case 1:
                ret = join_srv_wnd();
                break;
            case 2:
                ret = create_srv_wnd();
                break;
            case 3:
                ret = prefs_wnd();
                break;
            case -1:
                /* code */
                break;
            default:
                break;
        }
    }

    deinit_graphics();

    return 0;
}