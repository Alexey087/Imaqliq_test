#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <resolv.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "common.h"

void StartWork(const int port);
void sig_child(int sig);
void term_handler(int sig);
void report_write(const char *mes);
void failure_write(const char *mes);

int main(int argc, char* argv[])
{
    pid_t parpid;
    int port;

    if (argc != 2)   
    {
        fprintf(stderr, "usage:%s [port]\n", argv[0]);
        exit(1);
    }
    
    port = atoi(argv[1]);
    
    if ((parpid = fork()) < 0) 
    {   
        fprintf(stderr, "\ncan't fork");        
        exit(1);               
    }

    else if (parpid != 0)
        exit(0);

    umask(0);
    setsid();

    StartWork(port);

    return 0;
}

void StartWork(const int port)
{   
    char file_save_dir[MAXPATH] = "/tmp/";
    
    g_size_mes = sizeof(g_mes);
       
    int sockfd;
    int desc;   
    
    char host[MAXPATH];
    void sig_child(int);
    pid_t pid;
    
    struct sockaddr_in addr;         // socket parameters for bind
    char buf[BUF_SIZE];    
   
    long rc;
    
    socklen_t size = sizeof(struct sockaddr);
    
    sockfd = socket(PF_INET, SOCK_STREAM, 0);    
 
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY; // conect to any address
    addr.sin_port = htons(port);
    memset(&(addr.sin_zero),'\0',8);
      
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        failure_write("bind (maybe address is already in use?)");
        exit(0);
    }
    
    // signals processing add
    struct sigaction sa;
    sa.sa_handler = term_handler;
    
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGHUP, &sa, 0);
    
    listen(sockfd, 20);
    
    for (;;)
    {
        signal(SIGCHLD, sig_child);
        
        desc = accept(sockfd, (struct sockaddr *)&addr, &size);
        
        if (desc > 0)
        {
            if ((pid = fork()) == 0)
            {               
                strcpy(host, inet_ntoa(addr.sin_addr));

                snprintf(g_mes, g_size_mes, "Client by address %s connected", host);
                report_write(g_mes);            

                char file_name[MAXPATH];               
        
                // get the file name from the client
                rc = recv(desc, file_name, sizeof(file_name), 0);
                if (rc == -1)
                {                    
                    failure_write("recv failed");
                    exit(1);
                }

                snprintf(g_mes, g_size_mes, "ok");                
                if (send(desc, g_mes, strlen(g_mes), 0) == -1)
                    report_write("send confirm of file_name");
                
                char file_path[MAXPATH];           
                strcpy(file_path, file_save_dir);
                if (sizeof(file_name) <= sizeof(file_path))
                {                    
                    strncat(file_path, file_name, strlen(file_name));
                }                
                else
                {
                    failure_write("unable to append file name to file path");
                    exit(1);
                }
                
                snprintf(g_mes, g_size_mes, "Start receiving file: %s", file_path);                
                report_write(g_mes);
                                
                int fd = open(file_path, O_WRONLY | O_CREAT, 0777);
                if (fd == -1)
                {
                    failure_write("unable to open file for writing (maybe the directory doesn't exists?)");                    
                    exit(1);
                }
                
                do
                {                    
                    rc = read(desc, buf, BUF_SIZE);
                    if (rc == -1)
                    {
                        failure_write("read from socket");
                        exit(EXIT_FAILURE);
                    }
                    if (write(fd, buf, rc) == -1)
                    {
                        failure_write("write to file");
                        exit(EXIT_FAILURE);
                    }
                } 
                while (rc > 0);
                                
                snprintf(g_mes, g_size_mes, "End receiving file: %s\n\tClose session on client by address %s\n", file_path, host);
                report_write(g_mes);                
                
                close(desc);
                exit(0);
            }
            else if (pid > 0)
                close(desc);
        }
    }
}

void sig_child(int sig)
{
    pid_t pid;
    int stat;
    
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
    {
    }
    
    return;
}

void term_handler(int sig)
{    
    snprintf(g_mes, g_size_mes, "Terminating...\n\tsignal %i - %s\n", sig, sys_siglist[sig]);                
    report_write(g_mes);    
    
    exit(EXIT_SUCCESS);    
}

