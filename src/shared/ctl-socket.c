#include <stdlib.h>
#include <unistd.h>

#include "shared/ctl-socket.h"
#include "shared/log.h"
#include <errno.h>


/********************SERVER API***************************/

int setup_socket_server(tAPP_SOCKET *app_socket)
{
	unlink (app_socket->sock_path);
	if ((app_socket->server_sockfd = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) {
		error("fail to create socket: %s", strerror(errno));
		return -1;
	}
	app_socket->server_address.sun_family = AF_UNIX;
	strcpy (app_socket->server_address.sun_path, app_socket->sock_path);
	app_socket->server_len = sizeof (app_socket->server_address);
	app_socket->client_len = sizeof (app_socket->client_address);
	if ((bind (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, app_socket->server_len)) < 0) {
		error("bind: %s", strerror(errno));
		return -1;

	}
	if (listen (app_socket->server_sockfd, 10) < 0) {
		error("listen: %s", strerror(errno));
		return -1;
	}
	debug("Server is ready for client connect...\n");

	return 0;
}

int accpet_client(tAPP_SOCKET *app_socket)
{
	app_socket->client_sockfd = accept (app_socket->server_sockfd, (struct sockaddr *)&app_socket->server_address, (socklen_t *)&app_socket->client_len);
	if (app_socket->client_sockfd == -1) {
		error("accept: %s", strerror(errno));
		return -1;
	}
	return 0;
}

void teardown_socket_server(tAPP_SOCKET *app_socket)
{
	unlink (app_socket->sock_path);
	app_socket->server_sockfd = 0;
	app_socket->client_sockfd = 0;
}



/********************CLIENT API***************************/
int setup_socket_client(char *socket_path)
{
	struct sockaddr_un address;
	int sockfd,  len;

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
			error("%s", strerror(errno));
			return -1;
	}

	address.sun_family = AF_UNIX;
	strcpy (address.sun_path, socket_path);
	len = sizeof (address);

	if (connect (sockfd, (struct sockaddr *)&address, len) == -1) {
		debug("connect server: %s", strerror(errno));
		return -1;
	}

	return sockfd;

}

void teardown_socket_client(int sockfd)
{
	close(sockfd);
}

/********************COMMON API***************************/
int socket_send(int sockfd, char *msg, int len)
{
	int bytes;

	if (sockfd < 0) {
		error("%s", strerror(errno));
		return -1;
	}
	if ((bytes = send(sockfd, msg, len, 0)) < 0) {
		error("send: %s", strerror(errno));
	}
	return bytes;
}

int socket_recieve(int sockfd, char *msg, int len)
{
	int bytes;

	if (sockfd < 0) {
		error("%s", strerror(errno));
		return -1;
	}

	if ((bytes = recv(sockfd, msg, len, 0)) < 0)
	{
		error("recv: %s", strerror(errno));
	}
	return bytes;

}


