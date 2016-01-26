/*
 * Proxy Lab
 * Name: Jiaxing Hu
 * Andrew Id: jiaxingh
 * Description:
 * 
 */
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_SIZE 20 

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";



/* cache block */
struct cache_block {
    char data[MAX_OBJECT_SIZE];        /* store the response head and body */
    sem_t mutex;                    /* 模仿书上P673,使用写者-读者模型解决并发冲突 */
    sem_t w;
    int readcnt;

    int turn;                        /* the time of request */
    sem_t t_mutex;                
    sem_t t_w;
    int t_readcnt;

    char url[300];                    /* the url of request */
    sem_t url_mutex;
    sem_t url_w;
    int url_readcnt;

};
/* total CACHE_SIZE cache block */
struct cache_block cache[CACHE_SIZE];

/* init the block */
void cache_erase(int index);

/* because of concurrency, all operation on cache use following 4 function */
/* write data, url and turn on cache[index] */
void cache_write(int index, char *url,char *data);
/* read data on cache[index] */
void cache_data_read(int index, char *dst);
/* read url on cache[index] */
void cache_url_read(int index,char *dst);
/* read turn on cache[index] */
int cache_turn_read(int index);

/* thread for each request */
void *thread(void *connfdp);

/* parse the request line */
int parse_url(char *uri, char *host, char *filename);

/* connect to server, if failed return -1 */
int send_server(char *host,char *port,char *filename);

int is_static(char *query);
void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg);

int turn = 0;

int main(int argc,char **argv)
{
    //block the SIGPIPE
    Signal(SIGPIPE, SIG_IGN);

    //initialize
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* check port */
    if(argc!=2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // init CACHE_SIZE cache blocks
    int i;
    turn = 0;
    for(i=0;i<CACHE_SIZE;i++)
        cache_erase(i);

    listenfd=Open_listenfd(argv[1]);
    clientlen=sizeof(struct sockaddr_storage);

    while(1) {
        connfd = Accept(listenfd,(SA*)&clientaddr,&clientlen);
        Pthread_create(&tid,NULL,thread,(void *)&connfd);
    }
    return 0;
}

/* parse request url */
int parse_url(char *uri, char *host, char *filename)
{
    int port = 80;
    char *p = strstr(uri, "//");

    //initialize host and filename
    host[0] = '\0';
    filename[0] = '\0';

    // remove http://
    if (p != NULL) {
        sscanf(uri, "%*[^:]://%[^/]%s", host, filename);
    }
    else {
        sscanf(uri, "%[^/]%s", host, filename);
    }
    
    if(strlen(filename) == 0){
        strcpy(filename, "/");
    }
    //if the filename is /
    if (!strcmp(filename, "/")) {
        strcpy(filename,"/index.html");
    }

    // if there is a port
    p = strstr(host, ":");
    if (p != NULL) {
        sscanf(host, "%*[^:]:%d", &port);
        sscanf(host, "%[^:]", host);
    }
    return port;
}

/* connect to server and return socket fd */
int send_server(char *host,char *port,char *filename)
{
    int clientfd = 0;
    char buf[MAXLINE];

    clientfd = Open_clientfd(host, port);
    
    if (clientfd >= 0) {
        sprintf(buf, "GET %s HTTP/1.0\r\n", filename);
        sprintf(buf, "%sHost: %s\r\n", buf, host);
        sprintf(buf, "%s%s", buf, user_agent_hdr);
        sprintf(buf, "%s%s", buf, connection_hdr);
        sprintf(buf, "%s%s", buf, proxy_connection_hdr);
        sprintf(buf, "%s\r\n", buf);
        Rio_writen(clientfd,buf,strlen(buf));
    }
    return clientfd;
}

/*
 * if there is a finished cache, read and response.
 * else connect to server
 */
void *thread(void *p)
{
    //detach the thread at first
    Pthread_detach(pthread_self());

    //initialize
    int connfd = *(int *)p;
    char buf[MAXLINE];
    char method[MAXLINE],version[MAXLINE],url[MAXLINE];
    char host[MAXLINE],query[MAXLINE];
    rio_t rio;
    rio_t rio_s;
    int port;
    int length = 0;
    int serverfd;

    //int turn = 20;
    int index;
    char url_tmp[300],*data_tmp;

    /* read request line */
    Rio_readinitb(&rio,connfd);
    Rio_readlineb(&rio,buf,MAXLINE);
    printf("Request headers:%s \n", buf);
    sscanf(buf,"%s %s %s",method,url,version);

    //judge whether it is the GET method
    if (strcasecmp(method, "GET")) {
        clienterror(connfd, method, "501", "Not implemented",
                    "Proxy does not implement this method");
        Close(connfd);
        return NULL;
    }

    /* find cache block */
    for(index = 0;index < CACHE_SIZE;index++) {
        cache_url_read(index,url_tmp);
        /* the block'url is same as current url */
        if(!strcmp(url,url_tmp))
            break;
    }

    data_tmp=(char*)Malloc(MAX_OBJECT_SIZE);
    data_tmp[0]='\0';

    if(index < CACHE_SIZE) { /* if have cached */
        cache_data_read(index,data_tmp);
        Rio_writen(connfd,data_tmp,strlen(data_tmp));
        Close(connfd);
        free(data_tmp);
        return NULL;
    }

    //judge whether it is static or dynamic
    if (!is_static(url)) {
        free(data_tmp);
        Close(connfd);
        return NULL;
    }
    //connect to server
    port = parse_url(url,host,query);
    char port_str[5];
    sprintf(port_str, "%d", port);
    if((serverfd = send_server(host,port_str,query))<0) {
        // if failed
        free(data_tmp);
        Close(connfd);
        return NULL;
    }

    Rio_readinitb(&rio_s,serverfd);
    // read the header to get the length
    do {
        Rio_readlineb(&rio_s, buf, MAXLINE);
        strncat(data_tmp, buf, strlen(buf));
        //try to get the length of content
        if(strstr(buf,"Content-length") != NULL)
            sscanf(buf,"Content-length: %d\r\n",&length);
        Rio_writen(connfd,buf,strlen(buf));
    }while(strcmp(buf,"\r\n"));

    //read the body
    while(length > 0) {
        int counter = Rio_readnb(&rio_s,buf,(length<MAXLINE)?length:MAXLINE);
        length -= counter;
        strncat(data_tmp,buf,counter);
        Rio_writen(connfd,buf,counter);
    }
    index=0;
    int i;
    /* least-recently-used */
    for(i = 1;i < CACHE_SIZE;i++) {
        if(cache_turn_read(i)<cache_turn_read(index)) {
            index=i;
        }
    }
    // write to the cache
    cache_write(index,url,data_tmp);

    Close(connfd);
    Close(serverfd);
    free(data_tmp);
    return NULL;
}

int is_static(char *query)
{
    if (strstr(query, "?"))
        return 0;
    return 1;
}

void cache_erase(int index)
{
    /* init all var */
    cache[index].turn=0;    
    cache[index].url[0]='\0';
    cache[index].data[0]='\0';
    Sem_init(&cache[index].t_mutex,0,1);
    Sem_init(&cache[index].t_w,0,1);
    cache[index].t_readcnt=0;
    Sem_init(&cache[index].url_mutex,0,1);
    Sem_init(&cache[index].url_w,0,1);
    cache[index].url_readcnt=0;
    Sem_init(&cache[index].mutex,0,1);
    Sem_init(&cache[index].w,0,1);
    cache[index].readcnt=0;
}

void cache_write(int index,char *url, char *data)
{
    turn++;
    /* semaphore */
    P(&cache[index].url_w);
    P(&cache[index].w);
    P(&cache[index].t_w);
    /* begin write operation */
    cache[index].turn=turn;
    strcpy(cache[index].data,data);
    strcpy(cache[index].url,url);
    /* end write operation */

    /* semaphore */
    V(&cache[index].t_w);
    V(&cache[index].w);
    V(&cache[index].url_w);
    return ;
}

void cache_data_read(int index, char *dst)
{
    turn++;
    /* semaphore */
    P(&cache[index].mutex);
    cache[index].readcnt++;
    if(cache[index].readcnt == 1)
        P(&cache[index].w);
    V(&cache[index].mutex);
    P(&cache[index].t_w);

    /* begin read operation */
    cache[index].turn = turn;
    strcpy(dst,cache[index].data);
    /* end read operation */

    /* semphore */
    V(&cache[index].t_w);
    P(&cache[index].mutex);
    cache[index].readcnt--;
    if(cache[index].readcnt==0)
        V(&cache[index].w);
    V(&cache[index].mutex);

    return;
}

void cache_url_read(int index,char *dst)
{
    /* semaphore */
    P(&cache[index].url_mutex);
    cache[index].url_readcnt++;
    if(cache[index].url_readcnt==1)
        P(&cache[index].url_w);
    V(&cache[index].url_mutex);

    /* begin read operation */
    strcpy(dst,cache[index].url);
    /* end read operation */

    /* semphore */
    P(&cache[index].url_mutex);
    cache[index].url_readcnt--;
    if(cache[index].url_readcnt==0)
        V(&cache[index].url_w);
    V(&cache[index].url_mutex);

    return;
}

int cache_turn_read(int index)
{
    int t;
    /* semaphore */
    P(&cache[index].t_mutex);
    cache[index].t_readcnt++;
    if(cache[index].t_readcnt==1)
        P(&cache[index].t_w);
    V(&cache[index].t_mutex);

    /* begin read operation */
    t=cache[index].turn;
    /* end read operation */

    /* semphore */
    P(&cache[index].t_mutex);
    cache[index].t_readcnt--;
    if(cache[index].t_readcnt==0)
        V(&cache[index].t_w);
    V(&cache[index].t_mutex);

    return t;
}

void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}