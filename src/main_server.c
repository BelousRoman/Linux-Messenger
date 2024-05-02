#include "../hdr/network.h"

#define QUEUE_SIZE                      20

pthread_mutex_t fds_mutex = PTHREAD_MUTEX_INITIALIZER;

void *_processing_server_thread(void *args)
{
	struct pollfd pfd;
	struct client_msg_t msg;
    mqd_t fds_q;
    mqd_t broadcast_q;
    char queue_msg[QUEUE_SIZE+1];
    int srv_type = TYPE_NONE;
    int ret = 0;

    fds_q = mq_open("/main_server_fds", O_RDONLY);
    if (fds_q == -1)
    {
        perror("mq_open");
    }

	pfd.events = POLLIN;

	/* Set canceltype so thread could be canceled at any time*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(1)
    {
        if (mq_receive(fds_q, queue_msg, QUEUE_SIZE, NULL) == -1)
        {
            perror("mq_receive");
        }
        pfd.fd = strtol(queue_msg, NULL, 0);

        printf("Processing server will read the %dth fd\n", pfd.fd);

        while(1)
        {
            if (poll(&pfd, 1, 0) > 0)
            {
                if (pfd.revents & POLLIN)
                {
                    ret = recv(pfd.fd, &msg, sizeof(msg), 0);
                    if (ret == -1)
                    {
                        perror("recv");
                        break;
                    }
                    else if (ret == 0)
                    {
                        puts("Client disconnected");
                        break;
                    }
                    else
                    {
                        if (msg.command == PING_COMM)
                        {
                            puts("PING command received");
                            msg.command = PING_ANSW;

                            if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
                            {
                                perror("send");
                                break;
                            }
                        }
                        else if (msg.command == CONNECT_COMM)
                        {
                            printf("User: %s(%d) connected\n", msg.client_info.client_name, msg.client_info.id);

                            msg.command = CONNECT_ANSW;
                            strncpy(msg.client_info.client_name, "NewUser", sizeof(msg.client_info.client_name));
                            msg.client_info.id = 1;
                            msg.client_info.client_type = TYPE_USER;
                            msg.client_info.cur_server = 1;

                            if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
                            {
                                perror("send");
                                break;
                            }
                        }
                        else if (msg.command == JOIN_COMM)
                        {
                            printf("User: %s(%d) attempting to join server %s:%d\n", msg.join_srv.client_name, msg.join_srv.usr_id, msg.join_srv.ip, msg.join_srv.port);

                            // find server with this address and send connect request to it, change client state if attempt was success
                        }
                        else if (msg.command == CREATE_COMM)
                        {
                            printf("User"/*OPTIONAL: Find name by ID*/" #%d creating a server <%s>, address: %s:%d\n", msg.server_info.host_id, msg.server_info.server_name, msg.server_info.ip, msg.server_info.port);

                            // check if given address is free, allocate memory to server, send request
                        }
                        else if (msg.command == RENAME_COMM)
                        {
                            printf("User"/*OPTIONAL: Find name by ID*/" #%d requested rename operation to <%s>\n", msg.client_info.id, msg.client_info.client_name);
                        }
                        else if (msg.command == DISCONNECT_COMM)
                        {
                            if (srv_type != TYPE_SERVER)
                                break;
                            msg.command = DISCONNECT_ANSW;

                            if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
                            {
                                perror("send");
                                break;
                            }

                            puts("Shut down processing server");
                            break;
                        }
                        else if (msg.command == CLIENT_QUIT_COMM)
                        {
                            printf("User: %s(%d) disconnected\n", msg.client_info.client_name, msg.client_info.id);

                            msg.command = CLIENT_QUIT_ANSW;

                            if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
                            {
                                perror("send");
                                break;
                            }
                        }
                        else if (msg.command == SHUT_ROOM_COMM)
                        {
                            printf("User"/*OPTIONAL: Find name by ID*/"#%d shutting a server <%s>, address: %s:%d\n", msg.server_info.host_id, msg.server_info.server_name, msg.server_info.ip, msg.server_info.port);
                        }
                        else if (msg.command == SHUT_SRV_COMM)
                        {

                        }
                    }
                    pfd.revents = 0;
                }
                else if (pfd.revents & POLLHUP)
                {
                    puts("FD Closed");
                    break;
                }
            }
        }
    }
	
    mq_close(fds_q);
    exit(EXIT_SUCCESS);
}

int main_server()
{
    int server_fd;
    struct sockaddr_in server;
    int tmp_fd = 0;
    struct pollfd *client_pfds = NULL;
    int fds_len = MAIN_SERVER_LISTEN_BACKLOG;
    struct sockaddr_in client;
    int client_size;
	pthread_t *tid = NULL;
    mqd_t fds_q;
    mqd_t *broadcast_q;
    struct mq_attr attr;

    struct client_msg_t msg;
    char queue_msg[QUEUE_SIZE+1];

    int index;
    int srvs_num = 0;
    int ret = 0;

    // tid = malloc(tcp_alloc_threads * sizeof(pthread_t));
    // if (tid == NULL)
    // {
    //     printf("malloc: %s(%d)\n", strerror(errno), errno);
    //     exit(EXIT_FAILURE);
    // }

    attr.mq_maxmsg = 5;
	attr.mq_msgsize = sizeof(char) * QUEUE_SIZE;

    fds_q = mq_open("/main_server_fds", O_CREAT | O_RDWR, 0666, &attr);
    if (fds_q == -1)
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    pthread_create(&tid, NULL, _processing_server_thread, NULL);

    /* Fill 'client_pfds' and 'server' with 0's */
    // memset(client_pfds, 0, sizeof(struct pollfd) * fds_len);
	memset(&server, 0, sizeof(server));
    memset(&queue_msg, 0, sizeof(queue_msg)/sizeof(char));

	/* Set server's endpoint */
	server.sin_family = AF_INET;
	if (inet_pton(AF_INET, config.ip, &server.sin_addr) == -1)
	{
        printf("inet_pton: %s(%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
	}
	server.sin_port = config.port;

    printf("Server addr: %s : %d\n", config.ip, config.port);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd > 0)
    {
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

        if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) == -1)
		{
            printf("bind: %s(%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		}

        if (listen(server_fd, MAIN_SERVER_LISTEN_BACKLOG) == -1)
		{
            printf("listen: %s(%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		}

        while(1)
        {
            client_size = sizeof(client);
            if ((tmp_fd = accept(server_fd, (struct sockaddr *)&client,
                                    &client_size)) == -1)
            {
                printf("accept: %s(%d)\n", strerror(errno), errno);
                exit(EXIT_FAILURE);
            }

            snprintf(queue_msg, QUEUE_SIZE, "%d", tmp_fd);
            if (mq_send(fds_q, queue_msg, sizeof(QUEUE_SIZE), NULL) != 0)
            {
                perror("mq_send");
            }
        }
    }
    else
    {
        printf("socket: %s(%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    // if(client_pfds != NULL)
    // {
    //     free(client_pfds);
    // }
    // pthread_join(tid, NULL);
    mq_close(fds_q);
    unlink("/main_server_fds");

    return ret;
}