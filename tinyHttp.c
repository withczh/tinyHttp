
#include "common.h"
#include "tinyHttp.h"

//进一步封装rio_writen，处理sigpipe问题
void im_rio_writen(int fd,void *usrbuf,size_t n)
{
    if(rio_writen(fd,usrbuf,n)<0)
    {
        if(errno==EPIPE)
        {
            fprintf(stderr,"EPIPE error\n");
        }

        //unix_error("client side has ended connection");
    }
}

void sigchild_handler(int sig)
{
    int old_errno=errno;
    int stat;
    pid_t pid;
    while((pid=waitpid(-1,&stat,WNOHANG))>0){

    }
    errno=old_errno;
}

void doit(int fd)
{
    int is_static;
    struct stat stats;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    printf("Request line:\n");
    printf("%s",buf);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (!(strcasecmp(method, "GET")==0||strcasecmp(method,"POST")==0)) {
        clienterror(fd, method, "501", "Not Implemented",
                "Tiny does not implement this method");
        return;
    }
    int parm_len=read_requesthdrs(&rio,method);
    rio_readnb(&rio,buf,parm_len);

    is_static = parse_uri(uri, filename, cgiargs);
    if (stat(filename, &stats) < 0) {
        clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
        return;
    }

    if(is_static)
    {
        if(!(S_ISREG(stats.st_mode))||!(S_IRUSR&stats.st_mode))
        {
            printf("clienterror\n");
            clienterror(fd,filename,"403","Forbidden","Tiny cound't read this file");
            return;
        }
        printf("get into server_static...\n");
        server_static(fd,filename,stats.st_size);
    }
    else
    {

        if(!(S_ISREG(stats.st_mode))||!(stats.st_mode&S_IXUSR))
        {
            clienterror(fd,filename,"403","Forbidden","Tiny cound't run this cgi program");
            return;
        }
        printf("get into server_dynamic...\n");
        if(strcasecmp(method,"GET")==0){
            server_dynamic(fd,filename,cgiargs);
        }
        else{
            server_dynamic(fd,filename,buf);
        }   
    }
}  

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    im_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    im_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    im_rio_writen(fd, body, strlen(body));
    im_rio_writen(fd, buf, strlen(buf));
}

int read_requesthdrs(rio_t *rp,char *method)
{
    char buf[MAXLINE];
    int len=0;

    do
    {
        rio_readlineb(rp,buf,MAXLINE);
        printf("%s",buf);
        if(strcasecmp(method,"POST")==0&&strncasecmp(buf,"Content-Length:",15)==0){
            sscanf(buf,"Content-Length: %d",&len);
        }
    } while (strcmp(buf,"\r\n"));
    
    return len;
}

int parse_uri(char *uri,char *filename,char *cgiargs)
{
    char *ptr=NULL;

    if(!strstr(uri,"cgi-bin"))      //静态资源
    {
        strcpy(cgiargs,"");
        strcpy(filename,"./static");
        strcat(filename,uri);
        if(uri[strlen(uri)-1]=='/')
        {
            strcat(filename,"index.html");
        }
        return 1;
    }
    else    //动态资源
    {
        ptr=index(uri,'?');
        if(ptr!=NULL)
        {
            strcpy(cgiargs,ptr+1);
            *ptr='\0';
        }
        else
        {
            strcpy(cgiargs,"");
        }
        strcpy(filename,".");
        strcat(filename,uri);
        return 0;
    }
}

void server_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    //发送响应头给客户端
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    //sprintf(buf,"%sConnection: close\r\n",buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    im_rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s",buf);

    //发送响应体
    srcfd = open(filename, O_RDONLY, 0);
    srcp=(char*)malloc(filesize);
    rio_readn(srcfd,srcp,filesize);
    close(srcfd);
    im_rio_writen(fd,srcp,filesize);
    free(srcp);
}

void get_filetype(char *filename,char *filetype)
{
    if(strstr(filename,".html"))
    {
        strcpy(filetype,"text/html");
    }
    else if(strstr(filename,".gif"))
    {
        strcpy(filetype,"image/gif");
    }
    else if(strstr(filename,".png"))
    {
        strcpy(filetype,"image/png");
    }
    else if(strstr(filename,"jpg"))
    {
        strcpy(filetype,"image/jpeg");
    }
    else if(strstr(filename,".mp4"))
    {
        strcpy(filetype,"video/mp4");
    }
    else
    {
        strcpy(filetype,"text/plain");
    }
}

void server_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    im_rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    im_rio_writen(fd, buf, strlen(buf));

    if (fork() == 0) {
        setenv("QUERY_STRING", cgiargs, 1);
        dup2(fd, STDOUT_FILENO);
        execve(filename, emptylist, environ);
    }
}
