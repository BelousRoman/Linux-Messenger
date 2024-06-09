#ifndef _CONFIG_H
#define _CONFIG_H

#include "../libs/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <unistd.h>

#define STR_LEN                         20
#define IP_ADDR_LEN                     15
#define PORT_LEN                        5

#define LANGUAGE_ENGLISH    "en"
#define LANGUAGE_RUSSIAN    "ru"

#define LOCAL_IP_ADDR       "127.0.0.1"
#define PUBLIC_IP_ADDR      "192.168.0.11"

#define SRV_PORT            7889

#define DEFAULT_NAME        "user"
#define DEFAULT_ID          0
#define DEFAULT_LANGUAGE    LANGUAGE_ENGLISH
#define DEFAULT_IP_ADDR     LOCAL_IP_ADDR
#define DEFAULT_PORT        SRV_PORT

enum
{
    TYPE_BOOL = 1,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_OBJECT,
    TYPE_ARRAY
};

enum entries
{
    ENTRY_USERNAME = 1,
    ENTRY_ID,
    ENTRY_LANGUAGE,
    ENTRY_IP,
    ENTRY_PORT
};

enum
{
    LANG_EN = 1,
    LANG_RU
};

struct config_t
{
    int id;
    char name[STR_LEN+1];
    int language;
    char ip[IP_ADDR_LEN+1];
    unsigned short port;
};

extern struct config_t config;

int read_config();

int form_default_json(void);
int update_json_file(void);
int modify_config_entry(__uint8_t, void *);
int AddOrModifyEntry(char *, int, void *, ...);

#endif