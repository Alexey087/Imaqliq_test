#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "common.h"

void *get_in_addr(struct sockaddr *sa);
ssize_t send_file(long out_fd, long in_fd, size_t count);

int main(int argc, char **argv)
{
    int sockfd;     // socket decscriptor
    int port;        
    struct stat stat_buf;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char ip_addr[INET6_ADDRSTRLEN];
    int rc;         // return system code
    
    g_size_mes = sizeof(g_mes);
    
    if (argc != 4)
    {
        failure_write("usage: client hostname port file_path\n");        
        exit(1);
    }
    
    port = atoi(argv[2]);    
    if (port <= 0)
    {
        snprintf(g_mes, g_size_mes, "invalid port: %s", argv[2]);
        failure_write(g_mes);
        exit(1);
    }
    
    char strPort[10];
    
    if (snprintf(strPort, sizeof(strPort), "%d", port) == -1)
        return 1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
        
    if ((rv = getaddrinfo(argv[1], strPort, &hints, &servinfo)) != 0)
    {        
        snprintf(g_mes, g_size_mes,"getaddrinfo: %s", strerror(rv));
        failure_write(g_mes);
        return 1;
    }
    
    // loop through all the results and connect to the first we can
    for (p = servinfo; p!= NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            failure_write("client: socket");            
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {            
            failure_write("client: connect");
            close(sockfd);
            continue;
        }
        
        break;        
    }
    
    if (p == NULL)
    {
        failure_write("client: failed to connect");        
        return 2;
    }
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                ip_addr, sizeof ip_addr);
    
    snprintf(g_mes, g_size_mes, "client: connecting to %s", ip_addr);
    report_write(g_mes);
    
    freeaddrinfo(servinfo);     // all done with this structure
    
    const char *file_path = argv[3];    
    char *file_name = basename(strdup(file_path));    
        
    if (send(sockfd, file_name, strlen(file_name), 0) == -1)
        failure_write("send"); 

    char mes[100];    
    
    if( recv(sockfd, mes, sizeof(mes), 0) == -1) 
    {                    
       failure_write("recv failed"); 
       exit(1);
    }
    
    if (strncmp("ok", mes, 2) != 0)
    {
        failure_write(mes);
        exit(1);
    }
    
    snprintf(g_mes, g_size_mes, "client: start sending file '%s'", file_path );
    report_write(g_mes);
    
    int fd = open(file_path, O_RDONLY, 0777);
    
    if (fd == -1)
    {
        snprintf(g_mes, g_size_mes, "unable to open '%s': %s", file_path, strerror(errno));
        failure_write(g_mes);
        exit(1);
    }    
    
    // get the size of the file to be sent
    fstat(fd, &stat_buf);
    
    // copy file using send_file
    rc = send_file(sockfd, fd, stat_buf.st_size);
    if (rc == -1)
    {        
        snprintf(g_mes, g_size_mes, "error from send_file: %s", strerror(errno));
        failure_write(g_mes);
        exit(1);
    }
    
    if (rc != stat_buf.st_size)
    {        
        snprintf(g_mes, g_size_mes, "incomplete transfer from send_file: %s - %d of %ld bytes", strerror(errno), rc, (long)stat_buf.st_size);
        failure_write(g_mes);
        exit(1);
    }
    
    close(fd);
    close(sockfd);
    
    snprintf(g_mes, g_size_mes, "client: end sending file '%s'", file_path);
    report_write(g_mes);    
    
    return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)    
        return &(((struct sockaddr_in*)sa)->sin_addr);    
    else 
        return NULL;
}

ssize_t send_file(long out_fd, long in_fd, size_t count)
{    
    char buf[BUF_SIZE];
    long to_read, num_read, num_sent, total_sent;
    
    total_sent = 0;
    
    while (count > 0)
    {
        to_read = BUF_SIZE < count ? BUF_SIZE : count;
        
        num_read = read(in_fd, buf, to_read);
        if (num_read == -1)
            return -1;
        if (num_read == 0)
            break;          // EOF
        
        num_sent = write(out_fd, buf, num_read);
        if (num_sent == 0)                    
            failure_write("send_file: write() tranferred 0 bytes");        
            
        count += num_sent;
        total_sent += num_sent;        
    }
    
    return total_sent;    
}
