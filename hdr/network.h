#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <malloc.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <mqueue.h>

#include "configurator.h"

#define SERVER_LISTEN_BACKLOG           50
#define SERVER_THREADS_ALLOC            5

#define NET_SYSTEM_ERR                  2
#define NET_SOFTWARE_ERR                1

enum connection_status
{
    STATUS_DISCONNECTED,
    STATUS_CONNECTED,
    STATUS_CONNECTING
};

enum client_types
{
    TYPE_NONE,
    TYPE_USER,
    TYPE_SERVER
};

enum commands
{
    PING_COMM = 1,
    CONNECT_COMM,
    JOIN_COMM,
    CREATE_COMM,
    RENAME_COMM,
    DISCONNECT_COMM,
    CLIENT_QUIT_COMM,
    SHUT_ROOM_COMM,
    SHUT_SRV_COMM
};

enum message_status
{
    STATUS_COMMAND = 1,
    STATUS_REQUEST,
    STATUS_ANSWER,
    STATUS_ERROR
};

enum net_wait_mode
{
    WAIT_FALSE = 0,
    WAIT_TRUE
};

enum net_recv_mode
{
    RECV_BLOCK = 0,
    RECV_TIMEOUT,
    RECV_NONBLOCK
};

struct server_thread_t
{
    pthread_t tid;
    int fd;
    pthread_mutex_t mutex;
};

struct command_t
{
    uint8_t id;
    uint8_t status;
};

struct client_info_t
{
    char client_name[NAME_LEN+1];
    int id;
    int client_type;
    int cur_server;
};

struct server_info_t
{
    char server_name[NAME_LEN+1];
    char ip[IP_ADDR_LEN+1];
    int host_id;
    unsigned short port;
};

struct join_srv_t
{
    char client_name[NAME_LEN+1];
    char ip[IP_ADDR_LEN+1];
    int usr_id;
    unsigned short port;
};

struct error_t
{
    int errtype;
    int errnum;
};

struct client_msg_t
{
    struct command_t command;
    union
    {
        struct client_info_t client_info;
        struct server_info_t server_info;
        struct join_srv_t join_srv;
        struct error_t error;
    };
};

struct requests_t
{
    atomic_uchar ping_req;
    atomic_uchar connect_req;
    atomic_uchar join_req;
    atomic_uchar create_req;
    atomic_uchar rename_req;
    atomic_uchar disconnect_req;
    atomic_uchar shut_room_req;
    atomic_uchar shut_serv_req;
};

extern struct config_t config;

int main_server();

int connect_to_main_server(int *);
int is_connected(void);
int get_latency();
int client_send(int comm, int wait_flag, ...);
int client_recv(int comm, int mode);
int disconnect_from_main_server();

int connect_to_chat_server();

int chat_server();

#endif // _NETWORK_H