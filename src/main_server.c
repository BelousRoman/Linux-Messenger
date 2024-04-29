#include "../hdr/network.h"

pthread_mutex_t fds_mutex = PTHREAD_MUTEX_INITIALIZER;

void *_processing_server_thread(void *args)
{
	struct pollfd pfd;
	struct client_msg_t msg;
    mqd_t fds_q;
    int tmp_fd;

    if ((fds_q = mq_open("/main_server_fds", O_RDONLY)) == -1)
    {
        perror("mq_open");
    }
    puts("raz");
    if (mq_receive(fds_q, &tmp_fd, sizeof(tmp_fd)+1, NULL) == -1)
    {
        perror("mq_receive");
    }
    puts("dva");

    if (pthread_mutex_lock(&fds_mutex) != 0)
    {
        perror("mutex_lock");
    }
    pfd.fd = tmp_fd;
    ;if (pthread_mutex_unlock(&fds_mutex) != 0)
    {
        perror("mutex_unlock");
    }

	pfd.events = POLLIN;

	printf("Processing server will read the %dth fd\n", pfd.fd);

	/* Set canceltype so thread could be canceled at any time*/
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	while(1)
	{
		if (poll(&pfd, 1, 0) > 0)
		{
			memset(&msg, NULL, sizeof(msg));
			if (recv(pfd.fd, &msg, sizeof(msg), 0) == -1)
			{
				perror("recv");
				break;
			}
			else
			{
				printf("Message comm = %d\n", msg.command);

				if (msg.command == DISCONNECT_COMM)
				{
                    msg.command = DISCONNECT_ANSW;

                    if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
					{
						perror("send");
						break;
					}

					puts("Shut down processing server");
					break;
				}
				else if (msg.command == PING_COMM)
				{
                    msg.command = PING_ANSW;

					if (send(pfd.fd, &msg, sizeof(msg), 0) == -1)
					{
						perror("send");
						break;
					}
				}
			}
		}
	}
    mq_close(fds_q);
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
	pthread_t *tid;
    mqd_t fds_q;
    struct mq_attr attr;

    struct client_msg_t msg;

    int index;
    int ret = 0;

    // client_pfds = malloc(sizeof(struct pollfd) * fds_len);
    // if (client_pfds == NULL)
    // {
    //     printf("malloc: %s(%d)\n", strerror(errno), errno);
    //     exit(EXIT_FAILURE);
    // }

    attr.mq_maxmsg = 5;
	attr.mq_msgsize = sizeof(int);

    fds_q = mq_open("/main_server_fds", O_CREAT | O_RDWR, 0666, &attr);

    pthread_create(&tid, NULL, _processing_server_thread, NULL);

    /* Fill 'client_pfds' and 'server' with 0's */
    // memset(client_pfds, 0, sizeof(struct pollfd) * fds_len);
	memset(&server, 0, sizeof(server));

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

        client_size = sizeof(client);
		if ((tmp_fd = accept(server_fd, (struct sockaddr *)&client,
								&client_size)) == -1)
		{
            printf("accept: %s(%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		}
        msg.command = CONNECT_ANSW;
        send(tmp_fd, &msg, sizeof(msg), 0);
        puts("1");
        // pthread_create(&tid, NULL, _processing_server_thread, tmp_fd);
        pthread_mutex_lock(&fds_mutex);
        mq_send(fds_q, &tmp_fd, sizeof(tmp_fd), NULL);
        pthread_mutex_unlock(&fds_mutex);
        puts("2");
        // sleep(5);

        // printf("2 Client fd = %d\n", tmp_fd);
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
    mq_close(fds_q);
    unlink("/main_server_fds");

    return ret;
}