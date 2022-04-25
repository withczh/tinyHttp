#include "../common.h"

int main()
{
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = 0, n2 = 0;

    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        sscanf(buf,"first=%d",&n1);
        sscanf(p+1,"second=%d",&n2);
    }

    sprintf(content, "Welcome to add.com: ");
    sprintf(content, "%s The Internet addition portal .\r\n<p>", content);
    sprintf(content, "%s The answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
    sprintf(content, "%s Thanks for visiting!\r\n", content);

    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}
