#include "../hdr/configurator.h"

FILE *config_file = NULL;
cJSON * json = NULL;

struct config_t config;

int _create_config_file()
{
    int ret = 0;

    if (config_file != NULL)
        config_file = freopen("cfg.json", "w", config_file);
    else
        config_file = fopen("cfg.json", "w");
    
    if (config_file == NULL)
    {
        perror("fopen");
        ret = -1;
    }
    else
    {
        ret += form_default_json();
        ret += update_json_file();
    }

    return ret;
}

int read_config()
{
    int ret = 0;

    config_file = fopen("cfg.json", "r");
    if (config_file == NULL)
    {
        if (errno == ENOENT)
        {
            ret = _create_config_file();
        }
        else
        {
            perror("fopen");
            ret = 1;
        }
    }
    else
    {
        cJSON *language = cJSON_GetObjectItemCaseSensitive(json, "language"); 
        if (cJSON_IsString(language) && (language->valuestring != NULL))
        {
            if (strncmp(LANGUAGE_ENGLISH, language->valuestring, sizeof(LANGUAGE_ENGLISH)) == 0)
            {
                config.language = LANG_EN;
            }
            else if (strncmp(LANGUAGE_RUSSIAN, language->valuestring, sizeof(LANGUAGE_ENGLISH)) == 0)
            {
                config.language = LANG_RU;
            }
        }

        cJSON *ip = cJSON_GetObjectItemCaseSensitive(json, "server_ip"); 
        if (cJSON_IsString(ip) && (ip->valuestring != NULL))
        {
            strncpy(config.ip, ip->valuestring, sizeof(config.ip));
        }

        cJSON *port = cJSON_GetObjectItemCaseSensitive(json, "server_port"); 
        if (cJSON_IsNumber(port) && (port->valueint != NULL))
        {
            config.port = port->valueint;
        }
    }

    if (config_file != NULL)
        fclose(config_file);

    return ret;
}

int update_json_file(void)
{
    int ret = 0;
    
    char *json_str = cJSON_Print(json);
    if (json_str != NULL)
    {
        fputs(json_str, config_file);
        fflush(config_file);

        free(json_str);
    }
    else
        ret = 1;

    return ret;
}

int form_default_json(void)
{
    int ret = 0;

    json = cJSON_CreateObject();
    if (json != NULL)
    {
        ret += AddOrModifyEntry("language", TYPE_STRING, DEFAULT_LANGUAGE);
        ret += AddOrModifyEntry("server_ip", TYPE_STRING, DEFAULT_IP_ADDR);
        ret += AddOrModifyEntry("server_port", TYPE_INT, (void *)DEFAULT_PORT);
    }
    else
        ret = 1;

    // cJSON_AddStringToObject(new_json, "language", LANGUAGE_ENGLISH);
    // cJSON_AddStringToObject(new_json, "server_ip", LOCAL_IP_ADDR);
    // cJSON_AddNumberToObject(new_json, "server_port", SRV_PORT);

    return ret;
}

int AddOrModifyEntry(char * key, int value_type, void *value, ...)
{
    int ret = 0;

    if (json != NULL)
    {
        va_list ap;
        va_start(ap, value);
        // int a = va_arg(ap, int);
        // printf("a(%ld) = <%d>\n", &a, a);
        // char * a = va_arg(ap, char *);
        // printf("a(%ld) = <%s>\n", &a, a);
        switch (value_type)
        {
            case TYPE_INT:
                int int_val = (int)value;
                cJSON *json_int = cJSON_GetObjectItem(json, key);
                if (json_int != NULL)
                {
                    if (cJSON_IsNumber(json_int) == cJSON_True)
                    {
                        cJSON_ReplaceItemInObjectCaseSensitive(json, key, cJSON_CreateNumber(int_val));
                    }
                }
                else
                {
                    cJSON_AddNumberToObject(json, key, int_val);
                }
                break;
            case TYPE_STRING:
                char * str_val = (char *)value;
                if (str_val != NULL)
                {
                    cJSON *json_str = cJSON_GetObjectItem(json, key);
                    if (json_str != NULL)
                    {
                        if (cJSON_IsString(json_str) == cJSON_True)
                        {
                            cJSON_ReplaceItemInObjectCaseSensitive(json, key, cJSON_CreateString(str_val));
                        }
                    }
                    else
                    {
                        cJSON_AddStringToObject(json, key, str_val);
                    }
                }
                else
                {
                    ret = 1;
                }
                break;
            case TYPE_OBJECT:
                break;
            case TYPE_ARRAY:
                break;
            default:
                break;
        }

        va_end(ap);
    }
    else
    {
        ret = 1;
    }

    return ret;
}
