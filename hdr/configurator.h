#ifndef _CONFIG_H
#define _CONFIG_H

#include "../libs/cJSON.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

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
    TYPE_BOOL,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_OBJECT,
    TYPE_ARRAY
};

enum
{
    LANG_EN,
    LANG_RU
};

struct config_t
{
    int language;
    char ip[16];
    unsigned short port;
    char name[20];
    int id;
};

extern struct config_t config;

int read_config();

int form_default_json(void);
int update_json_file(void);
int AddOrModifyEntry(char *, int, void *, ...);

#endif