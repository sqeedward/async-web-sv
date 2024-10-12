// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/sendfile.h>
#include <sys/eventfd.h>
#include <libaio.h>
#include <errno.h>

#include "aws.h"
#include "utils/util.h"
#include "utils/debug.h"
#include "utils/sock_util.h"
#include "utils/w_epoll.h"

/* server socket file descriptor */
static int listenfd;

/* epoll file descriptor */
static int epollfd;

static io_context_t ctx;

static int aws_on_path_cb(http_parser *p, const char *buf, size_t len)
{
	struct connection *conn = (struct connection *)p->data;

	memcpy(conn->request_path, buf, len);
	conn->request_path[len] = '\0';
	conn->have_path = 1;

	return 0;
}

static void connection_prepare_send_reply_header(struct connection *conn)
{
	/* TODO: Prepare the connection buffer to send the reply header. */
}

static void connection_prepare_send_404(struct connection *conn)
{
	/* TODO: Prepare the connection buffer to send the 404 header. */
}

static enum resource_type connection_get_resource_type(struct connection *conn)
{
	/* TODO: Get resource type depending on request path/filename. Filename should
	 * point to the static or dynamic folder.
	 */
	return RESOURCE_TYPE_NONE;
}


struct connection *connection_create(int sockfd)
{
	struct connection *con = malloc(sizeof(struct connection));
	DIE(!con, "eroare la alocare");

	con->sockfd = sockfd;
	memset(con->recv_buffer, 0, BUFSIZ);
	memset(con->send_buffer, 0, BUFSIZ);

	return con;
}

void connection_start_async_io(struct connection *conn)
{
	/* TODO: Start asynchronous operation (read from file).
	 * Use io_submit(2) & friends for reading data asynchronously.
	 */
}

void connection_remove(struct connection *conn)
{
	/* TODO: Remove connection handler. */
}

void handle_new_connection(void)
{
	int sockfd = 0;
	struct sockaddr_in adresa;
	struct connection *con = NULL;
	socklen_t lungime = sizeof(struct sockaddr_in);
	int rc = 0;

	sockfd = accept(listenfd, (struct socaddr_in *) &adresa, &lungime);
	DIE(sockfd < 0, "accept");

	dlog(LOG_INFO, "Accepted connection from: %s:%d\n",inet_ntoa(adresa.sin_addr), ntohs(adresa.sin_port));
	
	/* TODO: Set socket to be non-blocking. */
	int flags = fcntl(sockfd, F_GETFL, 0);
	rc = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
	
	
	con = connection_create(sockfd);
	DIE(con == NULL, "eroare la conectare");

	rc = w_epoll_add_ptr_in(epollfd, sockfd, con);
	DIE(rc < 0 , "eroare la adaugare epool");

	/* TODO: Initialize HTTP_REQUEST parser. */
	rc = parse_header(con);
	DIE(rc != 0, "eroare la parser");
}

void receive_data(struct connection *conn)
{
	/* TODO: Receive message on socket.
	 * Store message in recv_buffer in struct connection.
	 */
	ssize_t bytes;
	int rc;
	char user[64];

	rc = get_peer_address(conn->sockfd, user, BUFSIZ);
	DIE(rc < 0, "eroare la user");
	
	bytes = recv(conn->sockfd, conn->recv_buffer, BUFSIZ, 0);
	DIE(bytes < 0, "eroare la bytes.");

	if (bytes == 0) {
		dlog(LOG_INFO, "Connection closed from: %s\n", user);
		rc = w_epoll_remove_ptr(epollfd, conn->fd, conn);
		conn->state = STATE_CONNECTION_CLOSED;
	}

	dlog(LOG_DEBUG, "Received message from: %s\n", user);

	printf("--\n%s--\n", conn->recv_buffer);

	conn->recv_len = bytes;
	conn->state = STATE_RECEIVING_DATA;

}

int connection_open_file(struct connection *conn)
{
	/* TODO: Open file and update connection fields. */

	return -1;
}

void connection_complete_async_io(struct connection *conn)
{
	/* TODO: Complete asynchronous operation; operation returns successfully.
	 * Prepare socket for sending.
	 */
}

int parse_header(struct connection *conn)
{
	/* TODO: Parse the HTTP header and extract the file path. */
	/* Use mostly null settings except for on_path callback. */
	http_parser_settings settings_on_path = {
		.on_message_begin = 0,
		.on_header_field = 0,
		.on_header_value = 0,
		.on_path = aws_on_path_cb,
		.on_url = 0,
		.on_fragment = 0,
		.on_query_string = 0,
		.on_body = 0,
		.on_headers_complete = 0,
		.on_message_complete = 0
	};
	return 0;
}

enum connection_state connection_send_static(struct connection *conn)
{
	/* TODO: Send static data using sendfile(2). */
	return STATE_NO_STATE;
}

int connection_send_data(struct connection *conn)
{
	/* May be used as a helper function. */
	/* TODO: Send as much data as possible from the connection send buffer.
	 * Returns the number of bytes sent or -1 if an error occurred
	 */
	return -1;
}


int connection_send_dynamic(struct connection *conn)
{
	/* TODO: Read data asynchronously.
	 * Returns 0 on success and -1 on error.
	 */
	return 0;
}


void handle_input(struct connection *conn)
{
	/* TODO: Handle input information: may be a new message or notification of
	 * completion of an asynchronous I/O operation.
	 */

	switch (conn->state) {
	default:
		printf("shouldn't get here %d\n", conn->state);
	}
}

void handle_output(struct connection *conn)
{
	/* TODO: Handle output information: may be a new valid requests or notification of
	 * completion of an asynchronous I/O operation or invalid requests.
	 */

	switch (conn->state) {
	default:
		ERR("Unexpected state\n");
		exit(1);
	}
}

void handle_client(uint32_t event, struct connection *conn)
{
	int rc;
	enum connection_state state;

	receive_data(conn);
	if (conn->state == STATE_CONNECTION_CLOSED)
		return;

	conn->send_len = conn->recv_len;
	memcpy(conn->send_buffer, conn->recv_buffer, conn->recv_len);

	rc = w_epoll_update_ptr_inout(epollfd, conn->sockfd, conn);
	DIE(rc < 0, "w_epoll_add_ptr_inout");

}

int main(void)
{
	int rc;

	/* TODO: Initialize asynchronous operations. */

	
	epollfd = w_epoll_create();
	DIE(epollfd < 0, "eroare la creare epoll");
	
	listenfd = tcp_create_listener(AWS_LISTEN_PORT, DEFAULT_LISTEN_BACKLOG);
	DIE(listenfd < 0 , "eroare la creare socket de listen.");

	
	rc = w_epoll_add_fd_in(epollfd, listenfd);
	DIE(rc < 0, "eroare adaugare socket in");

	dlog(LOG_INFO, "Server waiting for connections on port %d\n", AWS_LISTEN_PORT);

	/* O sa scriu procesul de gandire de acm, ca sa vad daca e clar:
	rev este o structura de care contine event-uri in srverul nostru,
	precum in pygame cu event. in event tine sub forma de numar, operatii
	pe biti, ce event s-a intamplat, in data sunt informatii despre user presupun.
	CUm ar fi, socket-ul lui, port-ul, numele, etc. */
	while (1) {
		struct epoll_event rev;

		rc = w_epoll_wait_infinite(epollfd, &rev);
		DIE(rc < 0, "eroare loop de event");

		if (rev.data.fd == listenfd) {
			dlog(LOG_DEBUG, "New connection\n");
			if (rev.events & EPOLLIN) {
				handle_new_connection();
			}
		}else {
			if (rev.events & EPOLLIN) {
				dlog(LOG_DEBUG, "New message\n");
				// handle_client_request(rev.data.ptr);
			}
		}
	}

	return 0;
}
