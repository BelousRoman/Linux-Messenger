#include "../hdr/network.h"

int main_server()
{
    int server_fd;
    struct sockaddr_in server;
    int tmp_fd = 0;
    int *client_fds = NULL;
    int fds_len = MAIN_SERVER_LISTEN_BACKLOG;
    struct sockaddr_in client;
    int client_size;

    int index;
    int ret = 0;

    client_fds = malloc(sizeof(int) * fds_len);
    if (client_fds == NULL)
    {
        printf("malloc: %s(%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    /* Fill 'server' with 0's */
	memset(&server, 0, sizeof(server));

	/* Set server's endpoint */
	server.sin_family = AF_INET;
	if (inet_pton(AF_INET, config.ip, &server.sin_addr) == -1)
	{
        printf("inet_pton: %s(%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
	}
	server.sin_port = config.port;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd > 0)
    {
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

        printf("accepting %d\n", tmp_fd);
        client_size = sizeof(client);
		if ((tmp_fd = accept(server_fd, (struct sockaddr *)&client,
								&client_size)) == -1)
		{
            printf("accept: %s(%d)\n", strerror(errno), errno);
			exit(EXIT_FAILURE);
		}

        printf("accepted %d\n", tmp_fd);

        sleep(15);

        printf("2 Client fd = %d\n", tmp_fd);
    }
    else
    {
        printf("socket: %s(%d)\n", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    if(client_fds != NULL)
    {
        free(client_fds);
    }

    return ret;
}