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
extern timer_t timer_id;
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

static void sigusr2_handler(int sig, siginfo_t *si, void *unused)
{
    timer_delete(timer_id);

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
                        // TODO Add dynamic status change on window upon losing connection with main server
                        mq_comm = CONNECT_COMM;
                        mq_send(mqd, &mq_comm, sizeof(int), NULL);
                    }
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
    struct sigaction sa_alrm;
    struct sigaction sa_usr1;
    struct sigaction sa_usr2;
    char mq_name[23];
    struct mq_attr attr;
    struct sigevent sev;
    int option = WND_MAIN_MENU;
    char server_name[STR_LEN+1];
    int run_flag = 1;
    int ret = EXIT_SUCCESS;
    init_graphics();
    // menu_wnd(&option);
    // popup_wnd("pop wait\nSample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text123 adfiygbqbe i qbfeq iifqe fgqeib viygdziyvgwrvb iuadgv biqv !@#??", POPUP_W_BLOCK);
    // popup_wnd("pop wait", POPUP_W_BLOCK);
    chat_wnd(&option, "Test chat lalalalala");
    // popup_wnd("pop wait", POPUP_W_BLOCK);
    // popup_wnd("Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text Sample text", POPUP_W_BLOCK);
    // menu_wnd(&option);
    // option = 1;
    // join_srv_wnd(&option);
    // option = 1;
    // create_srv_wnd(&option);
    // prefs_wnd();
    deinit_graphics();
    return 0;
    
    sigfillset(&sa_alrm.sa_mask);
    sa_alrm.sa_sigaction = sigalrm_handler;
    if (sigaction(SIGALRM, &sa_alrm, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&sa_usr1.sa_mask);
    sa_usr1.sa_sigaction = sigusr1_handler;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    sigfillset(&sa_usr2.sa_mask);
    sa_usr2.sa_sigaction = sigusr2_handler;
    if (sigaction(SIGUSR2, &sa_usr2, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    setlocale (LC_ALL, "");

    read_config();

    if (connect_to_main_server() != EXIT_FAILURE)
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
    while(run_flag)
    {
        switch (option)
        {
            case WND_NONE:
                run_flag = 0;
                break;
            case WND_MAIN_MENU:
                ret = menu_wnd(&option);
                break;
            case WND_JOIN_SRV:
                // if (connection_flag == STATUS_CONNECTED)
                    ret = join_srv_wnd(&option, server_name);
                // else
                //     popup_wnd("Not connected", POPUP_W_WAIT);
                break;
            case WND_CREATE_SRV:
                // if (connection_flag == STATUS_CONNECTED)
                    ret = create_srv_wnd(&option, server_name);
                // else
                //     popup_wnd("Not connected", POPUP_W_WAIT);
                break;
            case WND_PREFS:
                ret = cfg_wnd(&option);
                break;
            case WND_CHAT:
                ret = chat_wnd(&option, server_name);
            default:
                break;
        }

        if (ret != 0)
        {
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