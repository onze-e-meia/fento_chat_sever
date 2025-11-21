#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

const int	RECV_BUF	= 4096;
const char	*WNB_ARGS	= "Wrong number of arguments\n";
const char	*INV_ARG	= "Not a valid port\n";
const char	*FATAL		= "Fatal error\n";

typedef	struct sockaddr_in t_ip;

typedef struct	s_server {
	int		fd;
	int		max_fd;
	int		current_id;
}				t_server;

typedef struct	s_client {
	int		fd;
	int		id;
	char	*buffer;
	int		size;
}				t_client;

void	errorMsg(const char *msg, int server_fd) {
	write(STDERR_FILENO, msg, strlen(msg));
	if (server_fd > 0)
		close(server_fd);
	exit(1);
}

void	isDigit(char *str) {
	int		len = strlen(str);
	for (int i = 0; i < len; ++i ) {
		if (str[i] < '0' || str[i] > '9')
			errorMsg(INV_ARG, 0);
	}
}

void	broadcastMsg(char *msg, int exclude_fd, t_server *serv, fd_set *masterSet) {
	int		len = strlen(msg);
	for (int fd = 0; fd <= serv->max_fd; ++fd) {
		if (FD_ISSET(fd, masterSet) && fd != exclude_fd && fd != serv->fd)
			send(fd, msg, len, 0);
	}
}

void	newClient(t_client *clients, t_server *serv, fd_set *masterSet) {
	int			fd  = accept(serv->fd, NULL, NULL);

	if (fd < 0)
		return ;
	t_client	*cli = &clients[fd];
	cli->fd = fd;
	cli->id = (serv->current_id)++;
	cli->buffer = NULL;
	cli->size = 0;
	FD_SET(fd, masterSet);
	if (fd > serv->max_fd)
		serv->max_fd = fd;

	printf(" # New client accepted (id: %d | fd: %d).\n", cli->id, fd);
	char	msg[128];
	sprintf(msg, "server: client %d just arrived\n", cli->id);
	broadcastMsg(msg, cli->fd, serv, masterSet);
}

void	handleClients(t_client *cli, t_server *serv, fd_set *masterSet) {
	char	recvBuf[RECV_BUF + 1];
	int		bytes = recv(cli->fd, recvBuf, RECV_BUF, 0);

	if (bytes <= 0) {
		char	msg[128];
		sprintf(msg, "server: client %d just left\n", cli->id);
		broadcastMsg(msg, cli->fd, serv, masterSet);
		FD_CLR(cli->fd, masterSet);
		close(cli->fd);
		if (cli->buffer)
			free(cli->buffer);
		return ;
	}

	recvBuf[bytes] = '\0';
	if (!cli->buffer) {
		if (!(cli->buffer = malloc(bytes + 1)))
			errorMsg(FATAL, serv->fd);
		strcpy(cli->buffer, recvBuf);
		cli->size = bytes;
	} else {
		char	*newBuffer = realloc(cli->buffer, cli->size + bytes + 1);
		if (!newBuffer)
			errorMsg(FATAL, serv->fd);
		cli->buffer = newBuffer;
		strcat(cli->buffer, recvBuf);
		cli->size += bytes;
	}

	char	*newPos, *bufferStart = cli->buffer;
	while ((newPos = strstr(bufferStart, "\n"))) {
		*newPos = '\0';
		char	*msg = malloc(20 + strlen(bufferStart) + 2);
		if (!msg)
			errorMsg(FATAL, serv->fd);
		sprintf(msg, "client %d: %s\n", cli->id, bufferStart);
		broadcastMsg(msg, cli->fd, serv, masterSet);
		free(msg);
		bufferStart = newPos + 1;
	}

	if (*bufferStart) {
		int		remaining = strlen(bufferStart);
		char	*temp = malloc(remaining + 1);
		if (!temp)
			errorMsg(FATAL, serv->fd);
		strcpy(temp, bufferStart);
		free(cli->buffer);
		cli->buffer = temp;
		cli->size = remaining;
	} else {
		free(cli->buffer);
		cli->buffer = NULL;
		cli->size = 0;
	}
}

int		main(int argc, char *argv[]) {
	if (argc != 2)
		errorMsg(WNB_ARGS, 0);

	int	port = atoi(argv[1]);
	isDigit(argv[1]);
	printf(" # All good, connect PORT: %d\n", port);

	t_server	server;
	if ((server.fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		errorMsg(FATAL, 0);
	server.max_fd = server.fd;
	server.current_id = 0;

	t_ip	servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	servAddr.sin_port = htons(port);
	if (bind(server.fd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		errorMsg(FATAL, 0);
	if (listen(server.fd, 128) < 0 )
		errorMsg(FATAL, 0);

	fd_set	masterSet;
	FD_ZERO(&masterSet);
	FD_SET(server.fd, &masterSet);
	t_client	clients[4096];

	while (1) {
		fd_set	readSet = masterSet;

		if (select(server.max_fd + 1, &readSet, NULL, NULL, NULL) < 0)
			continue ;
		for (int fd = 0; fd <= server.max_fd; ++fd) {
			if (FD_ISSET(fd, &readSet)) {
				if (fd == server.fd)
					newClient(clients, &server, &masterSet);
				else
					handleClients(&clients[fd], &server, &masterSet);
			}
		}
	}
	close(server.fd);
	return 0;
}
