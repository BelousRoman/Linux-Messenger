#include "../hdr/network.h"

struct client_info_t client_info;
struct server_info_t server_info;

struct sockaddr_in server;
int server_fd = NULL;

struct timeval start_tv = {NULL,NULL};
struct timeval end_tv = {NULL,NULL};
int delayMcs = NULL;

pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;
int requests = 0;

int connect_to_main_server()
{
    struct client_msg_t msg;
	struct timespec ts;

    int index;
    int ret = EXIT_SUCCESS;

	/* Set 'ts' time to define-constants */
	ts.tv_sec = 0;
	ts.tv_nsec = 500000000;

	/* Fill 'server' with 0's */
	memset(&server, 0, sizeof(server));

	/* Set server's endpoint */
	server.sin_family = AF_INET;
	if (inet_pton(AF_INET, config.ip, &server.sin_addr) == -1)
	{
        printf("inet_pton: %s(%d)\n", strerror(errno), errno);
		return EXIT_FAILURE;
	}
	server.sin_port = config.port;

    // printf("Server addr: %s : %d\n", config.ip, config.port);

	/* Create socket */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        printf("socket: %s(%d)\n", strerror(errno), errno);
        return EXIT_FAILURE;
    }

	for (index = 0; index < 10; ++index)
	{
		if (connect(server_fd, (struct sockaddr *)&server, sizeof(server))
			== -1)
		{
            printf("connect: %s(%d)\n", strerror(errno), errno);
			if ((errno != ECONNREFUSED && errno != ENOENT) ||
				index == (10 - 1))
			{
				return EXIT_FAILURE;
			}
		}
		else
		{
			break;
		}
		nanosleep(&ts, NULL);
	}

    ret += client_send(CONNECT_COMM, WAIT_TRUE, RECV_TIMEOUT);

    return ret;
}

int check_connection_to_main_server()
{
    int ret = EXIT_SUCCESS;
    
    if (server_fd > 0)
    {
        ret = client_send(PING_COMM, WAIT_TRUE, RECV_BLOCK);
    }
    else
        ret = EXIT_FAILURE;

    return ret;
}

int get_latency()
{
    if (delayMcs == NULL && client_send(PING_COMM, WAIT_TRUE, RECV_BLOCK) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return delayMcs;
}

int client_send(int comm, int wait_flag, ...)
{
    // printf("%s\n", __func__);

    struct client_msg_t msg;
    va_list ap;
    int ret = EXIT_SUCCESS;

    va_start(ap, wait_flag);

    memset(&msg, NULL, sizeof(msg));

    msg.command.id = comm;
    msg.command.status = STATUS_REQUEST;

    switch (comm)
    {
    case PING_COMM:
        // msg.command.id = comm;

        if (wait_flag == WAIT_TRUE)
        {
            if (gettimeofday(&start_tv, NULL) == -1)
            {
                printf("gettimeofday: %s(%d)\n", strerror(errno), errno);
                ret = EXIT_FAILURE;
                break;
            }
        }

        pthread_mutex_lock(&req_mutex);
        requests |= PING_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        break;
    case CONNECT_COMM:
        // msg.command = comm;

        // msg.client_info.client_name = config.name;
        strncpy(msg.client_info.client_name, config.name, sizeof(msg.client_info.client_name));
        msg.client_info.client_type = TYPE_NONE;
        msg.client_info.cur_server = NULL;
        msg.client_info.id = config.id;

        pthread_mutex_lock(&req_mutex);
        requests |= CONNECT_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        break;
    case JOIN_COMM:
        if (client_info.id != NULL)
        {
            char * ip = va_arg(ap, char *);
            // printf("a(%ld) = <%s>\n", &ip, ip);
            int port = va_arg(ap, int);
            // printf("a(%ld) = <%d>\n", &port, port);

            // msg.command = comm;

            msg.join_srv.usr_id = client_info.id;
            // msg.join_srv.client_name = client_info.client_name;
            strncpy(msg.join_srv.client_name, client_info.client_name, sizeof(msg.join_srv.client_name));
            // msg.join_srv.ip = ip;
            strncpy(msg.join_srv.ip, ip, sizeof(msg.join_srv.ip));
            msg.join_srv.port = (unsigned short)port;

            pthread_mutex_lock(&req_mutex);
            requests |= JOIN_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    case CREATE_COMM:
        if (client_info.id != NULL)
        {
            char * name = va_arg(ap, char *);
            // printf("name(%ld) = <%s>\n", &name, name);
            char * ip = va_arg(ap, char *);
            // printf("ip(%ld) = <%s>\n", &ip, ip);
            int port = va_arg(ap, int);
            // printf("port(%ld) = <%d>\n", &port, port);

            // msg.command = comm;

            msg.server_info.host_id = client_info.id;
            // msg.server_info.server_name = name;
            // msg.server_info.ip = ip;
            strncpy(msg.server_info.server_name, name, sizeof(msg.server_info.server_name));
            strncpy(msg.server_info.ip, ip, sizeof(msg.server_info.ip));
            msg.server_info.port = (unsigned short)port;

            pthread_mutex_lock(&req_mutex);
            requests |= CREATE_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    case RENAME_COMM:
        if (client_info.id != NULL)
        {
            char * name = va_arg(ap, char *);
            // printf("name(%ld) = <%s>\n", &name, name);

            // msg.command = comm;

            // msg.client_info.client_name = name;
            strncpy(msg.client_info.client_name, name, sizeof(msg.client_info.client_name));
            msg.client_info.client_type = client_info.client_type;
            msg.client_info.cur_server = client_info.cur_server;
            msg.client_info.id = client_info.id;

            pthread_mutex_lock(&req_mutex);
            requests |= RENAME_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    case DISCONNECT_COMM:
        if (client_info.id != NULL)
        {
            // msg.command = comm;

            // msg.client_info.client_name = client_info.client_name;
            strncpy(msg.client_info.client_name, client_info.client_name, sizeof(msg.client_info.client_name));
            msg.client_info.client_type = client_info.client_type;
            msg.client_info.cur_server = client_info.cur_server;
            msg.client_info.id = client_info.id;

            pthread_mutex_lock(&req_mutex);
            requests |= DISCONNECT_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    case CLIENT_QUIT_COMM:
        if (client_info.id != NULL)
        {
            // msg.command = comm;

            // msg.client_info.client_name = client_info.client_name;
            strncpy(msg.client_info.client_name, client_info.client_name, sizeof(msg.client_info.client_name));
            msg.client_info.client_type = client_info.client_type;
            msg.client_info.cur_server = client_info.cur_server;
            msg.client_info.id = client_info.id;
        }
        else
            ret = EXIT_FAILURE;
        break;
    case SHUT_ROOM_COMM:
        if (client_info.id != NULL)
        {
            // msg.command = comm;

            msg.server_info.host_id = server_info.host_id;
            // msg.server_info.server_name = server_info.server_name;
            strncpy(msg.server_info.server_name, server_info.server_name, sizeof(msg.server_info.server_name));
            // msg.server_info.ip = server_info.ip;
            strncpy(msg.server_info.ip, server_info.ip, sizeof(msg.server_info.ip));
            msg.server_info.port = server_info.port;

            pthread_mutex_lock(&req_mutex);
            requests |= SHUT_ROOM_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    case SHUT_SRV_COMM:
        if (client_info.id != NULL)
        {
            // msg.command = comm;

            pthread_mutex_lock(&req_mutex);
            requests |= SHUT_SRV_REQUEST;
            pthread_mutex_unlock(&req_mutex);
        }
        else
            ret = EXIT_FAILURE;
        break;
    default:
        ret = EXIT_FAILURE;
        break;
    }

    if (ret == EXIT_SUCCESS)
    {
        if (send(server_fd, &msg, sizeof(msg), 0) == -1)
        {
            printf("send: %s(%d)\n", strerror(errno), errno);
            ret = EXIT_FAILURE;
        }

        if (wait_flag == WAIT_TRUE)
        {
            int recv_mode = va_arg(ap, int);
            ret = client_recv(comm, recv_mode);
        }
    }

    va_end(ap);

    return ret;
}

int client_recv(int comm, int mode)
{
    // printf("%s %d\n", __func__, comm);
    struct client_msg_t msg;
    int index;
    int ret = EXIT_SUCCESS;

    memset(&msg, NULL, sizeof(msg));

    if (server_fd <= 0)
    {
        return EXIT_FAILURE;
    }
    
    switch (mode)
    {
    case RECV_BLOCK:
        if (recv(server_fd, &msg, sizeof(msg), 0) == -1)
        {
            printf("recv: %s(%d)\n", strerror(errno), errno);
            return EXIT_FAILURE;
        }
        break;
    case RECV_TIMEOUT:
        struct timespec ts;
        /* Set 'ts' time to define-constants */
        ts.tv_sec = 0;
        ts.tv_nsec = 500000000;
        for (index = 0; index < 10; ++index)
        {
            if (recv(server_fd, &msg, sizeof(msg), MSG_DONTWAIT) == -1)
            {
                if (errno != EAGAIN)
                {
                    return EXIT_FAILURE;
                }
            }
            else
            {
                break;
            }
            nanosleep(&ts, NULL);
        }
        if (index == (10 - 1))
        {
            printf("recv: %s(%d)\n", strerror(errno), errno);
            return EXIT_FAILURE;
        }
        break;
    case RECV_NONBLOCK:
        if (recv(server_fd, &msg, sizeof(msg), MSG_DONTWAIT) == -1)
        {
            if (errno != EAGAIN)
            {
                printf("recv: %s(%d)\n", strerror(errno), errno);
            }
            return EXIT_FAILURE;
        }
        break;
    default:
        break;
    }

    // printf("Comm received: %d\n", msg.command);
    if (msg.command.status != STATUS_ANSWER)
    {
        return EXIT_FAILURE;
    }
    
    switch (msg.command.id)
    {
    case PING_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != PING_COMM) || !(requests & PING_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= PING_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        if (start_tv.tv_sec != NULL)
        {
            if (gettimeofday(&end_tv, NULL) == -1)
            {
                printf("gettimeofday: %s(%d)\n", strerror(errno), errno);
                ret = EXIT_FAILURE;
                break;
            }

            int startMcs, endMcs;
            startMcs = start_tv.tv_sec * (int)1e6 + start_tv.tv_usec;
            endMcs = end_tv.tv_sec * (int)1e6 + end_tv.tv_usec;
            delayMcs = endMcs - startMcs;
        }
        break;
    case CONNECT_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != CONNECT_COMM) || !(requests & CONNECT_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= CONNECT_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;

        ret += modify_config_entry(ENTRY_USERNAME, (char *)(client_info.client_name));
        ret += modify_config_entry(ENTRY_ID, &client_info.id);

        ret += update_json_file();
        break;
    case JOIN_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != JOIN_COMM) || !(requests & JOIN_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= JOIN_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;
        break;
    case CREATE_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != CREATE_COMM) || !(requests & CREATE_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= CREATE_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        server_info.host_id = msg.server_info.host_id;
        strncpy(server_info.server_name, msg.server_info.server_name, sizeof(server_info.server_name));
        strncpy(server_info.ip, msg.server_info.ip, sizeof(server_info.ip));
        server_info.port = msg.server_info.port;
        break;
    case RENAME_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != RENAME_COMM) || !(requests & RENAME_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= RENAME_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;

        ret += modify_config_entry(ENTRY_USERNAME, (char *)(client_info.client_name));

        ret += update_json_file();
        break;
    case DISCONNECT_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != DISCONNECT_COMM) || !(requests & DISCONNECT_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= DISCONNECT_REQUEST;
        pthread_mutex_unlock(&req_mutex);
        break;
    case CLIENT_QUIT_COMM:
        if (comm != NULL && comm != CLIENT_QUIT_COMM)
        {
            ret = EXIT_FAILURE;
            break;
        }
        break;
    case SHUT_ROOM_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != SHUT_ROOM_COMM) || !(requests & SHUT_ROOM_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= SHUT_ROOM_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;
        break;
    case SHUT_SRV_COMM:
        pthread_mutex_lock(&req_mutex);
        if ((comm != NULL && comm != SHUT_SRV_COMM) || !(requests & SHUT_SRV_REQUEST))
        {
            pthread_mutex_unlock(&req_mutex);
            ret = EXIT_FAILURE;
            break;
        }
        requests ^= SHUT_SRV_REQUEST;
        pthread_mutex_unlock(&req_mutex);

        disconnect_from_main_server();
        break;
    default:
        ret = EXIT_FAILURE;
        break;
    }

    return ret;
}

int disconnect_from_main_server()
{
    int ret = EXIT_SUCCESS;

    ret = client_send(CLIENT_QUIT_COMM, WAIT_TRUE, RECV_BLOCK);

    if (server_fd > 0)
        close(server_fd);

    return ret;
}
