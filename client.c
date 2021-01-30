#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#define WELCOME 1
#define REGISTER 2
#define LOGIN 3
#define MENU 4
#define TERMINATE 5

void handleMenu(){

}

int main(int argc, char *argv[]){
    int sockfd, len;
    char buf[BUFSIZ];
    struct sockaddr_in serv;
    int port;
    if(argc != 3){
        printf("Usage: ./prog host port\n");
        exit(1);
    }
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        exit(1);
    }
    serv.sin_family = PF_INET;
    port = strtol(argv[2], NULL, 10);
    serv.sin_port = htons(port);
    inet_aton(argv[1], &serv.sin_addr);

    if(connect(sockfd, (struct sockaddr*)&serv, sizeof(serv)) < 0){
        perror("connect");
        exit(1);
    }
    

    int menu=0;
    printf("Connected to server\n");
    memset(buf, 0, BUFSIZ);
    while(1){

        len = read(sockfd, buf, BUFSIZ);
        buf[len] = '\0';

        if(atoi(buf) == MENU){
            char *msg = "1";
            write(sockfd, msg, strlen(msg));
            // handleMenu(sockfd);
            //get files
            // len = read(sockfd, buf, BUFSIZ);
            // buf[len] = '\0';
            // printf("%s\n", buf);
            menu = MENU;
            continue;
        } 

        printf("%s\n", buf);
        fgets(buf, BUFSIZ, stdin);
        len = write(sockfd, buf, strlen(buf));
        

        if(menu == MENU){
            // printf("Upload File\n");
            buf[len] = '\0';
            if(atoi(buf) == 1){
                // printf("selected 1\n");
                len = read(sockfd, buf, BUFSIZ);
                buf[len] = '\0';
                printf("%s\n", buf);
            }
            if(atoi(buf) == 2){
                // printf("selected 2\n");
                int fd;
                char *filename;
                ssize_t length;
                int sent = 0, true_sent;
                char filesize[256];
                struct stat filestat;
                off_t offset;
                int remaining;

                read(sockfd, buf, BUFSIZ);
                printf("%s\n", buf);
                fgets(buf, BUFSIZ, stdin);
                buf[strlen(buf)-1] = '\0';
                write(sockfd, buf, strlen(buf));
                filename = buf;
                
                //file size
                fd = open(filename, O_RDONLY);
                if (fd == -1){
                    exit(1);
                }
                if (fstat(fd, &filestat) < 0){
                    exit(1);
                }

                sprintf(filesize, "%ld", filestat.st_size);

                //send file
                length = send(sockfd, filesize, sizeof(filesize), 0);
                if (length < 0){
                    exit(1);
                }

                offset = 0;
                remaining = filestat.st_size;
                /* Sending file data */
                while (remaining > 0) {
                        sent = sendfile(sockfd, fd, &offset, BUFSIZ);
                        
                        recv(sockfd, buf, BUFSIZ, 0);
                        fprintf(stdout, "True sent = %s\n", buf);

                        true_sent = atoi(buf);
                        if(sent != true_sent) {
                            offset -= sent;

                            sprintf(buf, "%d\0", 1);
                            send(sockfd, buf, BUFSIZ, 0);
                            continue;
                        }
                        sprintf(buf, "%d\0", 2);
                        send(sockfd, buf, BUFSIZ, 0);
                        remaining -= sent;
                }
            }
            if(atoi(buf) == 3) break;
        }
    }

    close(sockfd);
    return 0;
}