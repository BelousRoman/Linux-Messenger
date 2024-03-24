#include "../hdr/network.h"

struct sockaddr_in server;
int server_fd;

int connect_to_main_server()
{
    struct client_msg_t msg;
	struct timespec ts;

    int index;
    int ret = 0;

	/* Set 'ts' time to define-constants */
	ts.tv_sec = 0;
	ts.tv_nsec = 500000000;

	/* Fill 'server' with 0's */
	memset(&server, 0, sizeof(server));

	/* Set server's endpoint */
	server.sin_family = AF_INET;
	if (inet_pton(AF_INET, config.ip, &server.sin_addr) == -1)
	{
		perror("inet_pton");
		return EXIT_FAILURE;
	}
	server.sin_port = config.port;

	/* Create socket */
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

	for (index = 0; index < 10; ++index)
	{
		if (connect(server_fd, (struct sockaddr *)&server, sizeof(server))
			== -1)
		{
			perror("client connect");
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

    // msg.command = CONNECT_COMM;
    // msg.client_info
    // if (send(server_fd, msg, sizeof(msg), 0) == -1)
	// {
	// 	perror("Client send");
	// 	exit(EXIT_FAILURE);
	// }
    // sleep(15);

    // close(server_fd);

    return EXIT_SUCCESS;
}

int check_connection_to_main_server()
{
    if (server_fd > 0)
    {
        // send(server_fd, )
    }
    else
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int send_command(int comm)
{
    struct client_msg_t msg;

    msg.command = comm;

    return EXIT_SUCCESS;
}

int disconnect_from_main_server()
{
    if (server_fd > 0)
        close(server_fd);

    return EXIT_SUCCESS;
}
