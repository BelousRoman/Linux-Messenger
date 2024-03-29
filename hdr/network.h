#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "configurator.h"

#define MAIN_SERVER_LISTEN_BACKLOG      50
#define MAIN_SERVER_CLIENT_FD_ALLOC     10

#define NET_WAIT_TRUE                   1
#define NET_WAIT_FALSE                  0

#define NET_SYSTEM_ERR                  2
#define NET_SOFTWARE_ERR                1

enum client_types
{
    TYPE_NONE,
    TYPE_USER,
    TYPE_SERVER
};

enum commands
{
    PING_COMM = 0,
    CONNECT_COMM,
    JOIN_COMM,
    CREATE_COMM,
    RENAME_COMM,
    DISCONNECT_COMM,
    CLIENT_QUIT_COMM,
    SHUT_ROOM_COMM,
    SHUT_SRV_COMM
};

enum answers
{
    PING_ANSW = (SHUT_SRV_COMM+1),
    CONNECT_ANSW,
    JOIN_ANSW,
    CREATE_ANSW,
    RENAME_ANSW,
    DISCONNECT_ANSW,
    CLIENT_QUIT_ANSW,
    SHUT_ROOM_ANSW,
    SHUT_SRV_ANSW,
    ERROR_ANSW
};

struct client_info_t
{
    char client_name[20];
    int id;
    int client_type;
    int cur_server;
};

struct server_info_t
{
    char server_name[20];
    char ip[16];
    int host_id;
    unsigned short port;
};

struct join_srv_t
{
    char client_name[20];
    char ip[16];
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
    int command;
    union
    {
        struct client_info_t client_info;
        struct server_info_t server_info;
        struct join_srv_t join_srv;
        struct error_t error;
    };
};

extern struct config_t config;

int main_server();

int connect_to_main_server();
int check_connection_to_main_server();
int client_send(int comm, int wait_flag, ...);
int client_recv(int comm, int wait_flag, ...);
int disconnect_from_main_server();

int connect_to_chat_server();

int chat_server();

#endif // _NETWORK_H