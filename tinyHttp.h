#ifndef _TINYHTTP_H
#define _TINYHTTP_H

#include "common.h"

void doit(int fd);
int read_requesthdrs(rio_t *rp,char *method);
int parse_uri(char *uri,char *filename,char *cgiargs);
void get_filetype(char *filename,char *filetype);
void server_static(int fd,char *filename,int filesize);
void server_dynamic(int fd,char *filename,char *cgiargs);
void clienterror(int fd,char *cause,char *errnum,char *shortmsg,char *longmsg);
void sigchild_handler(int sig);

#endif