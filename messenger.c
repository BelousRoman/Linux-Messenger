#include "hdr/configurator.h"
#include "hdr/network.h"
#include "hdr/graphics.h"

#include <stdio.h>
#include <locale.h>

extern int is_connected;

float latency = 0;
// int is_connected = -1;

void *_net_thread(void *args)
{
    int ret = EXIT_SUCCESS;

    /* Set canceltype so thread could be canceled at any time*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    ret = connect_to_main_server();
    client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);

    if (is_connected)
    {
        latency = get_latency() / 1000.0;

        ret += client_send(JOIN_COMM, WAIT_FALSE, "255.255.255.255", 25519);
        ret += client_send(CREATE_COMM, WAIT_FALSE, "SERVERNAME", "127.127.127.127", 24321);
        ret += client_send(RENAME_COMM, WAIT_TRUE, "NEWUSRNAME", RECV_NONBLOCK);
        ret += client_send(RENAME_COMM, WAIT_TRUE, "Sample name", RECV_NONBLOCK);
    }

    while(1)
    {
        if (is_connected)
        {
            // TODO Add poll
            if (client_recv(NULL, RECV_NONBLOCK) != 0)
            {
                
                if (errno != EAGAIN)
                    return EXIT_FAILURE;
            }
        }
    }
}

int main(void)
{
    pthread_t tid;
    int ret = 1;
    setlocale (LC_ALL, "");

    read_config();
    init_graphics();

    wait_wnd("Privet", 1);
    pthread_create(&tid, NULL, _net_thread, NULL);

    if (1)
    {
        while(ret > 0)
        {
            switch (ret = menu_wnd())
            {
                case 1:
                    if (is_connected)
                    {
                        ret = join_srv_wnd();
                    }
                    else
                    {
                        wait_wnd("Not connected", 1);
                    }
                    break;
                case 2:
                    if (is_connected)
                    {
                        ret = create_srv_wnd();
                    }
                    else
                    {
                        wait_wnd("Not connected", 1);
                    }
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

    deinit_graphics();

    
    // printf("Latency = %d mcs\n", ret);

    // ret = 0;
    // ret += client_send(JOIN_COMM, WAIT_FALSE, "255.255.255.255", 25519);
    // ret += client_send(CREATE_COMM, WAIT_FALSE, "SERVERNAME", "127.127.127.127", 24321);
    // ret += client_send(RENAME_COMM, WAIT_TRUE, "NEWUSRNAME", RECV_TIMEOUT);
    // ret += client_send(RENAME_COMM, WAIT_TRUE, "Sample name", RECV_BLOCK);
    ret += disconnect_from_main_server();

    pthread_cancel(tid);
    
    printf("disconnected, latency = %g ms\n", latency);

    return 0;
}