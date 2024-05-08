#include "hdr/configurator.h"
#include "hdr/network.h"
#include "hdr/graphics.h"

#include <stdio.h>
#include <locale.h>

void *_net_thread(void *args)
{

}

int main(void)
{
    int ret = 1;
    setlocale (LC_ALL, "");

    read_config();
    init_graphics();

    // wait_wnd("Privet");

    if (connect_to_main_server() == EXIT_SUCCESS)
    {
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
    }
    int latency = get_latency();
    // printf("Latency = %d mcs\n", ret);

    ret = 0;
    ret += client_send(JOIN_COMM, WAIT_FALSE, "255.255.255.255", 25519);
    ret += client_send(CREATE_COMM, WAIT_FALSE, "SERVERNAME", "127.127.127.127", 24321);
    ret += client_send(RENAME_COMM, WAIT_TRUE, "NEWUSRNAME", RECV_TIMEOUT);
    ret += disconnect_from_main_server();
    

    deinit_graphics();
    
    printf("disconnected, latency = %d\n", latency);

    return 0;
}