#include "../hdr/network.h"

struct client_info_t client_info;
struct server_info_t server_info;

struct sockaddr_in server;
struct pollfd pfd = {0,0,0};

struct timeval start_tv = {NULL,NULL};
struct timeval end_tv = {NULL,NULL};
int delayMcs = NULL;

pthread_mutex_t recv_mutex = PTHREAD_MUTEX_INITIALIZER;
struct requests_t requests = {
    .ping_req = 0,
    .connect_req = 0,
    .join_req = 0,
    .create_req = 0,
    .rename_req = 0,
    .disconnect_req = 0,
    .shut_room_req = 0,
    .shut_serv_req = 0
};

int connection_flag = STATUS_DISCONNECTED;

int connect_to_main_server(int *fd)
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
	pfd.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (pfd.fd < 0)
    {
        printf("socket: %s(%d)\n", strerror(errno), errno);
        return EXIT_FAILURE;
    }

	for (index = 0; index < 10; ++index)
	{
		if (connect(pfd.fd, (struct sockaddr *)&server, sizeof(server))
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

    return ret;
}

int is_connected(void)
{
    return connection_flag;
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

        requests.ping_req++;

        break;
    case CONNECT_COMM:
        if (connection_flag != STATUS_DISCONNECTED)
            return EXIT_FAILURE;
        connection_flag = STATUS_CONNECTING;
        alarm(5);

        strncpy(msg.client_info.client_name, config.name, sizeof(msg.client_info.client_name));
        msg.client_info.client_type = TYPE_NONE;
        msg.client_info.cur_server = NULL;
        msg.client_info.id = config.id;

        requests.connect_req++;

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

            requests.join_req++;
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

            requests.create_req++;
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

            requests.rename_req++;
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

            requests.disconnect_req++;
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

            requests.shut_room_req++;
        }
        else
            ret = EXIT_FAILURE;
        break;
    case SHUT_SRV_COMM:
        if (client_info.id != NULL)
        {
            // msg.command = comm;

            requests.shut_serv_req++;
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
        if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
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

    if (pfd.fd <= 0)
    {
        return EXIT_FAILURE;
    }
    
    switch (mode)
    {
    case RECV_BLOCK:
        pthread_mutex_lock(&recv_mutex);
        if (recv(pfd.fd, &msg, sizeof(msg), 0) == -1)
        {
            printf("1 recv: %d %s(%d)\n", comm, strerror(errno), errno);
            pthread_mutex_unlock(&recv_mutex);
            return EXIT_FAILURE;
        }
        pthread_mutex_unlock(&recv_mutex);
        break;
    case RECV_TIMEOUT:
        struct timespec ts;
        /* Set 'ts' time to define-constants */
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000;
        pthread_mutex_lock(&recv_mutex);
        for (index = 0; index < 10; ++index)
        {
            if (recv(pfd.fd, &msg, sizeof(msg), MSG_DONTWAIT) == -1)
            {
                if (errno != EAGAIN)
                {
                    printf("2 recv %d: %s(%d)\n", comm, strerror(errno), errno);
                    pthread_mutex_unlock(&recv_mutex);
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
            printf("3 recv %d: %s(%d)\n", comm, strerror(errno), errno);
            pthread_mutex_unlock(&recv_mutex);
            return EXIT_FAILURE;
        }
        pthread_mutex_unlock(&recv_mutex);
        break;
    case RECV_NONBLOCK:
        if (pthread_mutex_trylock(&recv_mutex) != 0)
        {
            if (errno != EBUSY)
            {
                printf("4 recv %d: %s(%d)\n", comm, strerror(errno), errno);
                printf("is connected: %d\n", connection_flag);
            }
            return EXIT_FAILURE;
        }
        if (recv(pfd.fd, &msg, sizeof(msg), MSG_DONTWAIT) == -1)
        {
            if (errno != EAGAIN)
            {
                printf("5 recv %d: %s(%d)\n", comm, strerror(errno), errno);
                printf("is connected: %d\n", connection_flag);
            }
            pthread_mutex_unlock(&recv_mutex);
            return EXIT_FAILURE;
        }
        pthread_mutex_unlock(&recv_mutex);
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
        if ((comm != NULL && comm != PING_COMM) || requests.ping_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.ping_req--;

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
        if ((comm != NULL && comm != CONNECT_COMM) || requests.connect_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.connect_req--;

        alarm(0);
        connection_flag = STATUS_CONNECTED;
        if (kill(getpid(), SIGUSR1) != 0)
        {
            perror("kill");
            exit(EXIT_FAILURE);
        }

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;

        ret += modify_config_entry(ENTRY_USERNAME, (char *)(client_info.client_name));
        ret += modify_config_entry(ENTRY_ID, &client_info.id);

        ret += update_json_file();
        break;
    case JOIN_COMM:
        if ((comm != NULL && comm != JOIN_COMM) || requests.join_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.join_req--;

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;
        break;
    case CREATE_COMM:
        if ((comm != NULL && comm != CREATE_COMM) || requests.create_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.create_req--;

        server_info.host_id = msg.server_info.host_id;
        strncpy(server_info.server_name, msg.server_info.server_name, sizeof(server_info.server_name));
        strncpy(server_info.ip, msg.server_info.ip, sizeof(server_info.ip));
        server_info.port = msg.server_info.port;
        break;
    case RENAME_COMM:
        if ((comm != NULL && comm != RENAME_COMM) || requests.rename_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.rename_req--;

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;

        // printf("RENAME: %s\n", client_info.client_name);

        ret += modify_config_entry(ENTRY_USERNAME, (char *)(client_info.client_name));

        ret += update_json_file();
        break;
    case DISCONNECT_COMM:
        if ((comm != NULL && comm != DISCONNECT_COMM) || requests.disconnect_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.disconnect_req--;
        break;
    case CLIENT_QUIT_COMM:
        if (comm != NULL && comm != CLIENT_QUIT_COMM)
        {
            ret = EXIT_FAILURE;
            break;
        }
        break;
    case SHUT_ROOM_COMM:
        if ((comm != NULL && comm != SHUT_ROOM_COMM) || requests.shut_room_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.shut_room_req--;

        strncpy(client_info.client_name, msg.client_info.client_name, sizeof(client_info.client_name));
        client_info.client_type = msg.client_info.client_type;
        client_info.cur_server = msg.client_info.cur_server;
        client_info.id = msg.client_info.id;
        break;
    case SHUT_SRV_COMM:
        if ((comm != NULL && comm != SHUT_SRV_COMM) || requests.shut_serv_req <= 0)
        {
            ret = EXIT_FAILURE;
            break;
        }
        requests.shut_serv_req--;

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

    connection_flag = STATUS_DISCONNECTED;

    if (pfd.fd > 0)
    {
        pthread_mutex_lock(&recv_mutex);
        close(pfd.fd);
        pfd.fd = 0;
        pthread_mutex_unlock(&recv_mutex);
    }

    if (kill(getpid(), SIGUSR1) != 0)
    {
        perror("kill");
        exit(EXIT_FAILURE);
    }

    return ret;
}
