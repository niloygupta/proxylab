#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr =
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr =
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
sem_t proxy_mutex;

void *parse_request(void *connfd_arg);
void parse_url(char *read_buf, char *url, char *http_method, char *http_version,
		char *host, char *path, int *port);

void forward_request(int fd, char *url, char *host, char *path, int port);
void ignore_sigpipe(int arg);

int main(int argc, char **argv)
{
	int listenfd, *connfd, port;
	socklen_t clientlen;
	struct sockaddr_in clientaddr;
	struct hostent *hp;
	char *haddrp;
	pthread_t tid;
	Sem_init(&proxy_mutex, 0, 1);

	init_cache();

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
		connfd = Calloc(1, sizeof(int));
		*connfd = Accept(listenfd, (SA*) &clientaddr, &clientlen);
		 P(&proxy_mutex);
		/* Determine the domain name and IP address of the client */
		hp = Gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr,
				sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("server connected to %s (%s)\n", hp->h_name, haddrp);
		Pthread_create(&tid, NULL, parse_request, connfd);

		//parse_request(connfd);

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
void *parse_request(void *connfd_arg)
{
	int connfd = *((int *) connfd_arg);
	V(&proxy_mutex);
	Pthread_detach(pthread_self());
	Free(connfd_arg);

	size_t n;
	char read_buf[MAXLINE], http_method[MAXLINE], url[MAXLINE],
			http_version[MAXLINE], host[MAXLINE], path[MAXLINE];
	int port = 80;
	rio_t rio;

	Rio_readinitb(&rio, connfd);
	printf("\n Rio: %p",&rio);
	if ((n = Rio_readlineb(&rio, read_buf, MAXLINE)) == 0)
	{
		Close(connfd);
		return NULL ;
	}

	parse_url(read_buf, url, http_method, http_version, host, path, &port);

	if (!strncmp(read_buf, "https", strlen("https")))
		printf("https Not supported");

	forward_request(connfd, url, host, path, port);
	Close(connfd);
	return NULL ;
}


void forward_request(int fd, char *url, char *host, char *path, int port)
{
	rio_t rio;
	char buf[MAXBUF], http_response[MAXBUF];
	cache_line *cached_obj;
	printf("\n %s",url);
	cached_obj = get_cache_object(url);
	if(cached_obj!=NULL)
	{
		printf("\nURL!!: %s\n",cached_obj->uri);
		rio_writen(fd, cached_obj->content, cached_obj->length);
		update_cache(cached_obj);
		return;
	}
	printf("\n Opening connectiont to remote server");
	int server_fd = Open_clientfd(host, port);

	if (server_fd < -1)
	{
		printf("Invalid file descriptor received");
		return;
	}

	sprintf(buf, "GET %s HTTP/1.0\r\n", path);
	sprintf(buf, "%sHost: %s\r\n", buf, host);
	strcat(buf, user_agent_hdr);
	strcat(buf, accept_hdr);
	strcat(buf, accept_encoding_hdr);
	strcat(buf, "Connection: close\r\n");
	strcat(buf, "Proxy-Connection: close\r\n");
	strcat(buf,"\r\n");
	printf("HTTP request buf: \n%s\n", buf);

	Rio_writen(server_fd, buf, strlen(buf));

	//*buf ='\0';
	strcpy(http_response,"");
	strcpy(buf,"");
	Rio_readinitb(&rio, server_fd);

	int connfd;
	int cache_obj_size=0;
	char cache_obj[MAX_OBJECT_SIZE];
	strcpy(cache_obj,"");

	do
	{
		//*http_response = '\0';
		strcpy(http_response,"");
		connfd = Rio_readnb(&rio, http_response, MAXBUF);
		cache_obj_size +=connfd;
		if(cache_obj_size < MAX_OBJECT_SIZE)
					strcat(cache_obj,http_response);
		rio_writen(fd, http_response, connfd);


	} while (connfd > 0);
	if(cache_obj_size<MAX_OBJECT_SIZE)
		put_cache_object(url,cache_obj,cache_obj_size);
	return;
}

void ignore_sigpipe(int arg)
{
	printf("Signal Sigpipe received\n");
}

