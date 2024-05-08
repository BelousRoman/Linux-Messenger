#include "../hdr/configurator.h"

sem_t *cfg_file_sem = NULL;
cJSON * json = NULL;

struct config_t config;

int _create_config_file()
{
    int ret = EXIT_SUCCESS;

    ret += form_default_json();
    ret += update_json_file();

    return ret;
}

int read_config()
{
    FILE *config_file = NULL;
    int sem_value = 0;
    int ret = EXIT_SUCCESS;
    // unlink("/cfg_file_sem");
    cfg_file_sem = sem_open("/cfg_file_sem", O_RDWR);
    if (cfg_file_sem == SEM_FAILED)
    {
        if (errno == ENOENT)
        {
            cfg_file_sem = sem_open("/cfg_file_sem", O_CREAT | O_RDWR, 0666, 1);
            if (cfg_file_sem == SEM_FAILED)
            {
                perror("cfg_file_sem sem_open");
                return EXIT_FAILURE;
            }

            sem_getvalue(cfg_file_sem, &sem_value);
            while (sem_value > 1)
            {
                sem_trywait(cfg_file_sem);
                sem_getvalue(cfg_file_sem, &sem_value);
            }
        }
        else
        {
            perror("cfg_file_sem sem_open");
            return EXIT_FAILURE;
        }
    }
    sem_getvalue(cfg_file_sem, &sem_value);
    if (sem_value == 0) sem_post(cfg_file_sem);

    sem_wait(cfg_file_sem);
    config_file = fopen("cfg.json", "r+");
    if (config_file == NULL)
    {
        if (errno == ENOENT)
        {
            sem_post(cfg_file_sem);
            ret = _create_config_file();
        }
        else
        {
            perror("fopen");
            sem_post(cfg_file_sem);
            return EXIT_FAILURE;
        }
    }
    else
    {
        char buf[4097];
        memset(buf, NULL, 4097);
        if (fread(buf, sizeof(char), 4096, config_file) == 0)// TODO: add feof() and ferror()
        {
            fclose(config_file);
            sem_post(cfg_file_sem);
            ret = _create_config_file();
            return ret;
        }

        fclose(config_file);
        sem_post(cfg_file_sem);

        json = cJSON_Parse(buf);

        cJSON *language = cJSON_GetObjectItemCaseSensitive(json, "language"); 
        if (cJSON_IsString(language))
        {
            if (strncmp(LANGUAGE_ENGLISH, language->valuestring, sizeof(LANGUAGE_ENGLISH)) == 0)
            {
                config.language = LANG_EN;
            }
            else if (strncmp(LANGUAGE_RUSSIAN, language->valuestring, sizeof(LANGUAGE_ENGLISH)) == 0)
            {
                config.language = LANG_RU;
            }
            // printf("Language: %s(%d)\n", language->valuestring, config.language);
        }

        cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "username"); 
        if (cJSON_IsString(username))
        {
            // printf("Username: %s\n", username->valuestring);
            strncpy(config.name, username->valuestring, sizeof(config.name));
        }

        cJSON *id = cJSON_GetObjectItemCaseSensitive(json, "id"); 
        if (cJSON_IsNumber(id))
        {
            // printf("User ID %d\n", id->valueint);
            config.id = id->valueint;
        }

        cJSON *ip = cJSON_GetObjectItemCaseSensitive(json, "server_ip"); 
        if (cJSON_IsString(ip))
        {
            // printf("Server ip: %s\n", ip->valuestring);
            strncpy(config.ip, ip->valuestring, sizeof(config.ip));
        }

        cJSON *port = cJSON_GetObjectItemCaseSensitive(json, "server_port"); 
        if (cJSON_IsNumber(port))
        {
            // printf("Server port: %d\n", port->valueint);
            config.port = port->valueint;
        }
    }

    // printf("%s server addr: %s : %d\n", __func__, config.ip, config.port);
    // char *json_str = NULL;
    // json_str = cJSON_Print(json);
    // if (json_str != NULL)
    // {
    //     printf("json_str = <%s>\n", json_str);
    //     free(json_str);
    // }

    return ret;
}

int update_json_file(void)
{
    FILE *config_file = NULL;
    char *json_str = NULL;
    int ret = EXIT_SUCCESS;

    sem_wait(cfg_file_sem);
    config_file = fopen("cfg.json", "w+");
    if (config_file == NULL)
    {
        perror("fopen");
        sem_post(cfg_file_sem);
        return EXIT_FAILURE;
    }
    
    json_str = cJSON_Print(json);
    // printf("json_str = <%s>\n", json_str);
    if (json_str != NULL)
    {
        fputs(json_str, config_file);
        // printf("fputs ret = %d\n", fputs(json_str, config_file));
        if (fflush(config_file) != 0)
        {
            perror("fflush");
            ret += EXIT_FAILURE;
        }

        free(json_str);
    }
    else
        ret = EXIT_FAILURE;

    fclose(config_file);
    sem_post(cfg_file_sem);

    return ret;
}

int form_default_json(void)
{
    int ret = EXIT_SUCCESS;

    json = cJSON_CreateObject();
    if (json != NULL)
    {
        ret += AddOrModifyEntry("username", TYPE_STRING, DEFAULT_NAME);
        ret += AddOrModifyEntry("id", TYPE_INT, (void *)DEFAULT_ID);
        ret += AddOrModifyEntry("language", TYPE_STRING, DEFAULT_LANGUAGE);
        ret += AddOrModifyEntry("server_ip", TYPE_STRING, DEFAULT_IP_ADDR);
        ret += AddOrModifyEntry("server_port", TYPE_INT, (void *)DEFAULT_PORT);

        config.id = DEFAULT_ID;
        strncpy(config.ip, DEFAULT_IP_ADDR, sizeof(config.ip));
        config.language = LANG_EN;
        strncpy(config.name, DEFAULT_NAME, sizeof(config.name));
        config.port = DEFAULT_PORT;
    }
    else
        ret = EXIT_FAILURE;

    return ret;
}

int AddOrModifyEntry(char * key, int value_type, void *value, ...)
{
    int ret = EXIT_SUCCESS;

    if (json != NULL)
    {
        va_list ap;
        va_start(ap, value);
        switch (value_type)
        {
            case TYPE_INT:
                int int_val = (int)value;
                cJSON *json_int = cJSON_GetObjectItemCaseSensitive(json, key);
                if (json_int != NULL)
                {
                    if (cJSON_IsNumber(json_int))
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
                    cJSON *json_str = cJSON_GetObjectItemCaseSensitive(json, key);
                    if (json_str != NULL)
                    {
                        if (cJSON_IsString(json_str))
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
                    ret = EXIT_FAILURE;
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
        ret = EXIT_FAILURE;
    }

    return ret;
}

int modify_config_entry(__uint8_t entry, void *var)
{
    int ret = EXIT_SUCCESS;

    switch (entry)
    {
    case ENTRY_USERNAME:
        char *username = (char *)var;
        strncpy(config.name, username, NAME_LEN);

        ret += AddOrModifyEntry("username", TYPE_STRING, config.name);
        break;
    case ENTRY_ID:
        int *id = (int *)var;
        config.id = *id;

        ret += AddOrModifyEntry("id", TYPE_INT, config.id);
        break;
    case ENTRY_LANGUAGE:
        if (LANG_EN == (int *)var)
        {
            config.language = LANG_EN;
            ret += AddOrModifyEntry("language", TYPE_STRING, LANGUAGE_ENGLISH);
        }
        else if (LANG_RU == (int *)var)
        {
            config.language = LANG_RU;
            ret += AddOrModifyEntry("language", TYPE_STRING, LANGUAGE_RUSSIAN);
        }
        break;
    case ENTRY_IP:
        char *ip = (char *)var;
        strncpy(config.ip, ip, IP_ADDR_LEN);

        ret += AddOrModifyEntry("server_ip", TYPE_STRING, config.ip);
        break;
    case ENTRY_PORT:
        unsigned short *port = (unsigned short *)var;
        config.port = *port;

        ret += AddOrModifyEntry("server_port", TYPE_INT, config.port);
        break;
    default:
        ret = EXIT_FAILURE;
        break;
    }
    return ret;
}