#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr =
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr =
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

void parse_request(int connfd);
void parse_url(char *read_buf, char *url, char *http_method, char *http_version,
		char *host, char *path, int *port);
void extract_headers(rio_t *rp, char *host_header, char *other_headers);
void forward_request(int fd, char *url, char *host, char *path, char *host_header,
		char *other_headers, int port);
void ignore_sigpipe(int arg);

int main(int argc, int **argv)
{
	int listenfd, connfd, port;
	socklen_t clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp;
	signal(SIGPIPE, ignore_sigpipe);
	if (argc != 2)
	{
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}

	port = atoi(argv[1]);
	listenfd = Open_listenfd(port);
	while (1)
	{
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);

		/* Determine the domain name and IP address of the client */
		hp = Gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr,
				sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("server connected to %s (%s)\n", hp->h_name, haddrp);

		parse_request(connfd);
		Close(connfd);
	}
	return 0;
}
void parse_url(char *read_buf, char *url, char *http_method, char *http_version,
		char *host, char *path, int *port)
{
	*path = '/';
	*(path + 1) = '\0';
	sscanf(read_buf, "%s %s %s", http_method, url, http_version);
	sscanf(url, "http://%s", host);

	char *extract_host = strchr(host, '/');
	if (extract_host != NULL )
	{
		strcpy(path, extract_host);
		*extract_host = '\0';
	}

	char *find_port = strchr(host, ':');
	if (find_port != NULL )
	{
		*port = atoi(find_port + 1);
		*find_port = '\0';
	}

}
void parse_request(int connfd)
{
	size_t n;
	char read_buf[MAXLINE], http_method[MAXLINE], url[MAXLINE],
			http_version[MAXLINE],host[MAXLINE], path[MAXLINE],
			headers[MAXLINE], other_headers[MAXLINE];
	int port = 80;
	rio_t rio;

	Rio_readinitb(&rio, connfd);
	if ((n = Rio_readlineb(&rio, read_buf, MAXLINE)) == 0)
		return;

	extract_headers(&rio, headers, other_headers);

	parse_url(read_buf, url, http_method, http_version, host, path, &port);

	// make a new string so as not to defile original url

	if (!strncmp(read_buf, "https", strlen("https")))
		printf("https Not supported");

	forward_request(connfd, url, host, path, headers, other_headers, port);

	//Ends here
}

void extract_headers(rio_t *rp, char *host_header, char *other_headers)
{
	char buf[MAXLINE];
	*other_headers = "";
	*(other_headers + 1) = '\0';

	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n"))
	{
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);

		int prefix = strlen("Host: ");

		if (!strncmp(buf, "Host: ", prefix))
			strcpy(host_header, buf + prefix);
		/* We add other headers when required */
		if (strncmp(buf, "User-Agent: ", strlen("User-Agent: "))
				&& strncmp(buf, "Accept: ", strlen("Accept: "))
				&& strncmp(buf, "Accept-Encoding: ",
						strlen("Accept-Encoding: "))
				&& strncmp(buf, "Connection: ", strlen("Connection: "))
				&& strncmp(buf, "Proxy-Connection: ",
						strlen("Proxy-Connection: ")))
		{
			sprintf(other_headers, "%s%s", other_headers, buf);
		}
		/* We added this check in order to ignore garbage headers */
		if (buf[0] > 90 || buf[0] < 65)
		{
			return;
		}
	}
	return;
}

void forward_request(int fd, char *url, char *host, char *path, char *host_header,
		char *other_headers, int port)
{
	int server_fd;
	char buf[MAXBUF], http_response[MAXBUF];
	rio_t rio;

	server_fd = Open_clientfd(host, port);

	if (server_fd < 0)
	{
		printf("Invalid file descriptor received");
		return;
	}

	/* The following code adds the necessary information to make buf a complete request */
	sprintf(buf, "GET %s HTTP/1.0\r\n", path);
	printf("Send request buf: \n%s\n", buf);

	if (!strlen(host_header))
		sprintf(buf, "%sHost: %s\r\n", buf, host);
	else
		sprintf(buf, "%sHost: %s\r\n", buf, host_header);
	strcat(buf, user_agent_hdr);
	strcat(buf, accept_hdr);
	strcat(buf, accept_encoding_hdr);
	strcat(buf, "Connection: close\r\n");
	strcat(buf, "Proxy-Connection: close\r\n");
	strcat(buf,other_headers);
	strcat(buf,"\r\n");


	Rio_writen(server_fd, buf, strlen(buf));

	strcpy(http_response, "");
	strcpy(buf, "");
	Rio_readinitb(&rio, server_fd);

	int connfd;
	do
	{
		*http_response = '\0';
		connfd = Rio_readnb(&rio, http_response, MAXBUF);
		rio_writen(fd, http_response, connfd);

	} while (connfd > 0);

	return;
}

void ignore_sigpipe(int arg)
{
	printf("Signal Sigpipe received\n");
}

