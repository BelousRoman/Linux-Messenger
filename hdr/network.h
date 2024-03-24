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

#include "configurator.h"

#define MAIN_SERVER_LISTEN_BACKLOG      50
#define MAIN_SERVER_CLIENT_FD_ALLOC     10

enum client_types
{
    TYPE_NONE,
    TYPE_USER,
    TYPE_SERVER
};

enum commands
{
    PING_COMM,
    PING_ANSW,
    RENAME_COMM,
    RENAME_ANSW,
    CONNECT_COMM,
    CONNECT_ANSW,
    CREATE_COMM,
    CREATE_ANSW,
    SHUT_SRV_COMM,
    SHUT_SRV_ANSW,
    DISCONNECT_COMM
};

struct client_info_t
{
    int client_type;
    int id;
    int cur_server;
    char name[20];
};

struct server_info_t
{
    int host_id;
    char name[20];
    char ip[16];
    unsigned short port;
};

struct client_msg_t
{
    int command;
    union
    {
        struct client_info_t client_info;
        struct server_info_t server_info;
    };
};

extern struct config_t config;

int main_server();

int connect_to_main_server();
int check_connection_to_main_server();
int disconnect_from_main_server();

int connect_to_chat_server();

int chat_server();

#endif // _NETWORK_H