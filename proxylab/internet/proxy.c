/*
 * Proxy Lab
 * Name: Jiaxing Hu
 * Andrew Id: jiaxingh
 * Description:
 * First, proxy read the request from client.
 * Then, parse the request to find the url.
 * if url is cached, return the cached page.
 * if not, send the request to the server, and get the page.
 * return the page to client and cache it.
 * I used a struct to save the data of cache, and two array
 * to save the url and turn of cache. 
 * In cache, I used the reader-writer model defined in the textbook
 * to achieve concurrency. The data can be read by many clients at one
 * time, so I add the readcnt to count the number.
 * I also used the turn to find the least-used block.
 * 
 */
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_SIZE 20 
#define URL_SIZE 200

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";



//this is the block of the cache
struct cache_block {
    //store the data
    char data[MAX_OBJECT_SIZE]; 
    //store the number of reader
    int readcnt;
    //use sem to achieve concurrency
    sem_t mutex;
    sem_t w;
};


/* init the block */
int cache_initialize();

/* write basic info to cache block */
void cache_write(int index, char *url,char *data);

/* read the data in block to dst */
void cache_data_read(int index, char *dst);

/* find the least-used index in cache */
int cache_find();

/* this is the main routine for every request */
void *thread(void *connfdp);

/* parse the uri to host and filename */
int parse_url(char *uri, char *host, char *filename);

/* send request to server */
int send_server(char *host,char *port,char *filename);

/* judge whether it requires static url */
int is_static(char *query);

/* this is the error printer in textbook */
void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg);

// declare CACHE_SIZE blocks of cache
//save the data
struct cache_block cache[CACHE_SIZE];
//save the url
char cache_url[CACHE_SIZE][URL_SIZE];
//save the turn
int cache_turn[CACHE_SIZE];
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

    // check port 
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // initialize CACHE_SIZE cache blocks
    int result = cache_initialize();
    if (result < 0) {
        fprintf(stderr, "error in initialize the cache");
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    while(1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd,(SA*)&clientaddr,&clientlen);
        Pthread_create(&tid,NULL,thread,(void *)&connfd);
    }
    return 0;
}

// given uri, return host name and file name
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

// send the request to server
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

// This is the main routina for each thread
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

    int is_found = 0;
    int index;
    char *data;

    // read the request from client
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

    // search url in cache block
    for(index = 0;index < CACHE_SIZE;index++) {
        //judge whether find it
        if(!strcmp(url,cache_url[index])) {
            is_found = 1;
            break;
        }
    }

    // this is the need data
    data = (char*)Malloc(MAX_OBJECT_SIZE);
    data[0] = '\0';

    //judge whether find the object url
    if(is_found == 1) {
        cache_data_read(index,data);
        Rio_writen(connfd,data,strlen(data));
        Close(connfd);
        free(data);
        return NULL;
    }

    //judge whether it is static or dynamic
    if (!is_static(url)) {
        clienterror(connfd, method, "501", "Dynamic page",
                    "Proxy cannot serve dynamic page");
        free(data);
        Close(connfd);
        return NULL;
    }

    //connect to server
    port = parse_url(url,host,query);
    char port_str[5];
    sprintf(port_str, "%d", port);
    if((serverfd = send_server(host,port_str,query))<0) {
        // if failed
        clienterror(connfd, method, "501", "Server error",
                    "Proxy does not find this server");
        free(data);
        Close(connfd);
        return NULL;
    }

    //initialize the rio of server
    Rio_readinitb(&rio_s,serverfd);

    // read the header
    do {
        Rio_readlineb(&rio_s, buf, MAXLINE);
        strncat(data, buf, strlen(buf));
        //try to get the length of content
        if(strstr(buf,"Content-length") != NULL)
            sscanf(buf,"Content-length: %d\r\n",&length);
        Rio_writen(connfd,buf,strlen(buf));
    } while (strcmp(buf,"\r\n"));

    //read the body
    while (length > 0) {
        //using the flag to avoid the error when the file is binary
        int flag_length = 1;
        if (MAXLINE < length)
            flag_length = 0;
        int counter;
        if (flag_length == 1)
            counter = Rio_readnb(&rio_s,buf,length);
        else
            counter = Rio_readnb(&rio_s,buf,MAXLINE);
        length -= counter;
        strncat(data,buf,counter);
        Rio_writen(connfd,buf,counter);
    }

    //now save it to cache
    // I use least-used method to manage the cache
    index = cache_find();
    // write to the cache
    cache_write(index,url,data);

    Close(connfd);
    Close(serverfd);
    free(data);
    return NULL;
}

//check whether it is the static page
int is_static(char *query)
{
    if (strstr(query, "?"))
        return 0;
    return 1;
}

//write to cache
void cache_write(int index,char *url, char *data)
{
    turn++;
    // semaphore 
    P(&cache[index].w);

    // write operation
    strcpy(cache[index].data,data);
    strcpy(cache_url[index],url);
    cache_turn[index] = turn;

    // semaphore 
    V(&cache[index].w);
    return ;
}

//read from cache
void cache_data_read(int index, char *dst)
{
    turn++;
    // semaphore 
    P(&cache[index].mutex);
    cache[index].readcnt++;
    if(cache[index].readcnt == 1)
        P(&cache[index].w);
    V(&cache[index].mutex);

    // read operation 
    strcpy(dst,cache[index].data);
    cache_turn[index] = turn;

    // semphore 
    P(&cache[index].mutex);
    cache[index].readcnt --;
    if(cache[index].readcnt == 0)
        V(&cache[index].w);
    V(&cache[index].mutex);

    return;
}

//initialize the cache
int cache_initialize() 
{
    int index = 0;
    turn = 0;
    while (index < CACHE_SIZE) {
        strcpy(cache[index].data, "");
        cache[index].readcnt = 0;
        Sem_init(&cache[index].mutex,0,1);
        Sem_init(&cache[index].w,0,1);
        strcpy(cache_url[index], "");
        cache_turn[index] = turn;
        index ++;
    }
    return 1;
}

//find the least-used cache block
int cache_find() 
{
    int answer = 0;
    int turn1 = cache_turn[0];
    int i;
    for (i = 0; i < CACHE_SIZE; i++) {
        if (turn1 > cache_turn[i]) {
            turn1 = cache_turn[i];
            answer = i;
        }
    }
    return answer;
}

//This is the error printer from textbook
void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    // Build the HTTP response body 
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Web server</em>\r\n", body);

    // Print the HTTP response 
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}