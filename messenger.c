#include "hdr/configurator.h"
#include "hdr/network.h"
#include "hdr/graphics.h"

#include <stdio.h>
#include <locale.h>
#include <pthread.h>
#include <poll.h>
#include <signal.h>
#include <mqueue.h>

extern struct config_t config;
extern int connection_flag;
extern struct pollfd pfd;
mqd_t mqd = 0;
int mq_comm = 0;

float latency = 0;

static void sigalrm_handler(int sig, siginfo_t *si, void *unused)
{
    switch (connection_flag)
    {
    case STATUS_CONNECTING:
        connection_flag = STATUS_DISCONNECTED;
        break;
    default:
        break;
    }
    if (mqd > 0)
    {
        mq_comm = CONNECT_COMM;
        mq_send(mqd, &mq_comm, sizeof(int), NULL);
    }
    return;
}

static void sigusr1_handler(int sig, siginfo_t *si, void *unused)
{
    switch (connection_flag)
    {
    case STATUS_CONNECTING:
        connection_flag = STATUS_DISCONNECTED;
        break;
    default:
        break;
    }
    if (mqd > 0)
    {
        mq_comm = CONNECT_COMM;
        mq_send(mqd, &mq_comm, sizeof(int), NULL);
    }
    return;
}

void *_net_thread(void *args)
{
    int ret = EXIT_SUCCESS;

    /* Set canceltype so thread could be canceled at any time*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    pfd.events = POLLIN;

    if (connection_flag)
    {
        latency = get_latency() / 1000.0;

        ret += client_send(JOIN_COMM, WAIT_FALSE, "255.255.255.255", 25519);
        ret += client_send(CREATE_COMM, WAIT_FALSE, "SERVERNAME", "127.127.127.127", 24321);
        ret += client_send(RENAME_COMM, WAIT_FALSE, "NEWUSRNAME");
        ret += client_send(RENAME_COMM, WAIT_FALSE, "Sample name");
    }

    while(1)
    {
        switch (connection_flag)
        {
        case STATUS_CONNECTED:
            if (poll(&pfd, 1, 0) > 0)
            {
                if (pfd.revents & POLLIN)
                {
                    if (client_recv(NULL, RECV_TIMEOUT) != 0)
                    {
                        popup_wnd(strerror(errno), POPUP_W_WAIT);
                        connection_flag = STATUS_DISCONNECTED;
                        if (errno == ECONNRESET)
                        {
                            connection_flag = STATUS_DISCONNECTED;
                            // popup_wnd(strerror(errno), POPUP_W_WAIT);
                            // TODO Add dynamic status change on window upon losing connection with main server
                            mq_comm = CONNECT_COMM;
                            mq_send(mqd, &mq_comm, sizeof(int), NULL);
                            
                        }
                        // switch (is_connected())
                        // {
                        // case STATUS_DISCONNECTED:
                        //     if (kill(getpid(), SIGUSR1) != 0)
                        //     {
                        //         perror("kill");
                        //         exit(EXIT_FAILURE);
                        //     }
                        //     break;
                        // default:
                        //     break;
                        // }
                        
                    }
                }
                pfd.revents = 0;
            }
            break;
        default:
            break;
        }
    }
}

int main(void)
{
    pthread_t tid;
    struct sigaction sa;
    char mq_name[23];
    struct mq_attr attr;
    struct sigevent sev;
    int option = 1;
    int ret = EXIT_SUCCESS;
    init_graphics();
    menu_wnd(&option);
    option = 1;
    join_srv_wnd(&option);
    option = 1;
    create_srv_wnd();
    deinit_graphics();
    return 0;
    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = sigalrm_handler;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = sigusr1_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    setlocale (LC_ALL, "");

    read_config();

    connect_to_main_server();
    client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);

    snprintf(mq_name, 22, "/client%d_ntg", config.id);
    mq_unlink(mq_name);
    attr.mq_maxmsg = 5;
	attr.mq_msgsize = sizeof(int);
    
    mqd = mq_open(mq_name, O_CREAT | O_RDWR, 0666, &attr);
    if (mqd == -1)
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    sev.sigev_notify = SIGEV_THREAD;
	sev.sigev_notify_function = handle_msg;
	sev.sigev_notify_attributes = NULL;
	sev.sigev_value.sival_int = mqd;

    if (mq_notify(mqd, &sev) == -1)
	{
		perror("mq_notify");
		exit(EXIT_FAILURE);
	}

    init_graphics();

    // popup_wnd(mq_name, 1);
    // char ch[11];
    // snprintf(ch, 10, "%d", mqd);
    // popup_wnd(ch, 1);

    pthread_create(&tid, NULL, _net_thread, NULL);
    ret = 1;
    while(option > 0)
    {
        ret = menu_wnd(&option);
        switch (option)
        {
            case 1:
                if (connection_flag)
                {
                    ret = join_srv_wnd(&option);
                }
                else
                {
                    popup_wnd("Not connected", POPUP_W_WAIT);
                }
                break;
            case 2:
                if (connection_flag)
                {
                    // ret = create_srv_wnd();
                }
                else
                {
                    popup_wnd("Not connected", POPUP_W_WAIT);
                }
                break;
            case 3:
                // ret = prefs_wnd();
                break;
            case -1:
                /* code */
                break;
            default:
                break;
        }
    }
    
    pthread_cancel(tid);

    ret += disconnect_from_main_server();
    
    deinit_graphics();
    
    printf("User#%d \"%s\" disconnected, latency = %g ms\n", config.id, config.name, latency);

    unlink(mq_name);

    return 0;
}