#include "common.h"
#define SERV_PORT 8888

void unix_error(char *msg) /* 错误处理 */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

/****************************************
 * The Rio package - Robust I/O functions
 ****************************************/

/*
 * rio_readn - Robustly read n bytes (unbuffered)
 */
/* $begin rio_readn */
ssize_t rio_readn(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nread = read(fd, bufp, nleft)) < 0) {
	    if (errno == EINTR) /* Interrupted by sig handler return */
		nread = 0;      /* and call read() again */
	    else
		return -1;      /* errno set by read() */ 
	} 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* Return >= 0 */
}
/* $end rio_readn */

/*
 * rio_writen - Robustly write n bytes (unbuffered)
 */
/* $begin rio_writen */
ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
	if ((nwritten = write(fd, bufp, nleft)) <= 0) {
	    if (errno == EINTR)  /* Interrupted by sig handler return */
		nwritten = 0;    /* and call write() again */
	    else
		return -1;       /* errno set by write() */
	}
	nleft -= nwritten;
	bufp += nwritten;
    }
    return n;
}
/* $end rio_writen */


/* 
 * rio_read - This is a wrapper for the Unix read() function that
 *    transfers min(n, rio_cnt) bytes from an internal buffer to a user
 *    buffer, where n is the number of bytes requested by the user and
 *    rio_cnt is the number of unread bytes in the internal buffer. On
 *    entry, rio_read() refills the internal buffer via a call to
 *    read() if the internal buffer is empty.
 */
/* $begin rio_read */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    while (rp->rio_cnt <= 0) {  /* Refill if buf is empty */
	rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, 
			   sizeof(rp->rio_buf));
	if (rp->rio_cnt < 0) {
	    if (errno != EINTR) /* Interrupted by sig handler return */
		return -1;
	}
	else if (rp->rio_cnt == 0)  /* EOF */
	    return 0;
	else 
	    rp->rio_bufptr = rp->rio_buf; /* Reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;          
    if (rp->rio_cnt < n)   
	cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}
/* $end rio_read */

/*
 * rio_readinitb - Associate a descriptor with a read buffer and reset buffer
 */
/* $begin rio_readinitb */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;  
    rp->rio_cnt = 0;  
    rp->rio_bufptr = rp->rio_buf;
}
/* $end rio_readinitb */

/*
 * rio_readnb - Robustly read n bytes (buffered)
 */
/* $begin rio_readnb */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;
    
    while (nleft > 0) {
	if ((nread = rio_read(rp, bufp, nleft)) < 0) 
            return -1;          /* errno set by read() */ 
	else if (nread == 0)
	    break;              /* EOF */
	nleft -= nread;
	bufp += nread;
    }
    return (n - nleft);         /* return >= 0 */
}
/* $end rio_readnb */

/* 
 * rio_readlineb - Robustly read a text line (buffered)
 */
/* $begin rio_readlineb */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) { 
        if ((rc = rio_read(rp, &c, 1)) == 1) {
	    *bufp++ = c;
	    if (c == '\n') {
                n++;
     		break;
            }
	} else if (rc == 0) {
	    if (n == 1)
		return 0; /* EOF, no data read */
	    else
		break;    /* EOF, some data was read */
	} else
	    return -1;	  /* Error */
    }
    *bufp = 0;
    return n-1;
}

/*信号量的封装*/
void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if(sem_init(sem,pshared,value)<0)
    {
        unix_error("sem init error");
    }
}
void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
	unix_error("P error");
}
void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
	unix_error("V error");
}
/******/

/*有限缓存区*/
void sbuf_init(sbuf_t *sp,int capacity)
{
    sp->buf=calloc(capacity,sizeof(int));
    sp->capacity=capacity;
    sp->size=0;
    sp->front=0;
    sp->rear=capacity-1;
    Sem_init(&sp->mutex,0,1);
    Sem_init(&sp->slots,0,capacity);
    Sem_init(&sp->items,0,0);
}
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}
void sbuf_insert(sbuf_t *sp,int item)
{
    P(&sp->slots);
    P(&sp->mutex);
    
    sp->rear=(sp->rear+1)%sp->capacity;
    sp->buf[sp->rear]=item;
    sp->size=sp->size+1;
    
    V(&sp->mutex);
    V(&sp->items);
}
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);
    P(&sp->mutex);
    
    item=sp->buf[sp->front];
    sp->front=(sp->front+1)%sp->capacity;
    sp->size=sp->size-1;

    V(&sp->mutex);
    V(&sp->slots);

    return item;
}
int sbuf_is_empty(sbuf_t *sp)
{
    int t;
    P(&sp->mutex);
    t=(sp->size==0);
    V(&sp->mutex);
    return t;
}
int sbuf_is_full(sbuf_t *sp)
{
    int t;
    P(&sp->mutex);
    t=(sp->size==sp->capacity);
    V(&sp->mutex);
    return t;
}
/****/

/*建立监听套接字*/
int make_socket()
{
    int listenfd;
    struct sockaddr_in serveraddr;

    listenfd=socket(AF_INET,SOCK_STREAM,0);
    if(listenfd<0)
    {
        unix_error("make socket mailed");
    }

    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(SERV_PORT);
    serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);

    int opt=1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    if((bind(listenfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)))<0)
    {
        unix_error("bind failed");
    }

    listen(listenfd,1024);
    return listenfd;
}
/*****/