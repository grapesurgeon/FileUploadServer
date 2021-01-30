#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h> 
#include <dirent.h>
#include <fcntl.h>


int welcome(int sockfd);
int reg(int sockfd);
int login(int sockfd);
void menu (int sockfd, int id);
int searchUser(char *user, int mode);
int checkLogin(char *user, char *pass);

int main(int argc, char *argv[]){
    int sockfd, len, new_sockfd;
    char buf[BUFSIZ];
    struct sockaddr_in serv, clnt;
    socklen_t sin_siz;
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
    sin_siz = sizeof(struct sockaddr_in);
    if(bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0){
        perror("bind");
        exit(1);
    }
    if(listen(sockfd, SOMAXCONN) < 0){
        perror("listen");
        exit(1);
    }

    pid_t cpid;
    while(1){
        if((new_sockfd = accept(sockfd, (struct sockaddr *)&clnt, &sin_siz)) < 0){
            perror("accept");
        }

        cpid = fork();

        if(cpid < 0){
            perror("fork");
        } else if(cpid == 0){
            close(sockfd);
            printf("connect from %s: %d\n", inet_ntoa(clnt.sin_addr), ntohs(clnt.sin_port));
            memset(buf, 0, BUFSIZ);
            
            int id = welcome(new_sockfd);
            menu(new_sockfd, id);

            close(new_sockfd);
            exit(1);
        }
        close(new_sockfd);
        
    }
    close(sockfd);
    return 0;
}

int welcome(int sockfd){
    printf("Welcome started\n");
    int choose, id = 0, len;
    char *msg;
    char buf[BUFSIZ];
    memset(buf, 0, BUFSIZ);

    while(1){
        msg = "Welcome to online storage\n1. Register\n2. Login\n3. Exit\nInput: ";
        write(sockfd, msg, strlen(msg));
        len = read(sockfd, buf, BUFSIZ);
        buf[len] = '\0';
        choose = atoi(buf);

        if(choose == 1){
            id = reg(sockfd);
            break;
        }
        else if(choose == 2){
            id = login(sockfd);
            break;
        }
        else if(choose == 3){
            exit(1);
        }
    }

    if(id == 0){
        printf("Connection terminated\n");
        exit(1); 
    }   

    return id;
}

int reg(int sockfd){
    char buf[BUFSIZ];
    char *user, *pass, *msg;
    int id = 0, len;
    memset(buf, 0, BUFSIZ);

    while(1){
        msg = "Username [3-20 char]: ";
        write(sockfd, msg, strlen(msg));
        len = read(sockfd, buf, BUFSIZ);
        buf[len-1] = '\0'; // remove newline
        user = strdup(buf);
        //printf("Received username: %s", user);

        if(strlen(user) > 2 && strlen(user) < 21 && (id = searchUser(user, 1)) != 0){
            break;
        }
    }
    // printf("id = %d\n", id);
    
    msg = "Password: ";
    write(sockfd, msg, strlen(msg));
    len = read(sockfd, buf, BUFSIZ);
    buf[len-1] = '\0';
    pass = strdup(buf);
    //printf("Received password: %s\n", pass);

    //create id
    if(id == 0) id++;
    char uid[5];
    int c=0, temp = id;

    while(temp != 0){
        temp /= 10;
        c++;
    }
    if(c==1){
        sprintf(uid, "000%d", id);
    }
    if(c==2){
        sprintf(uid, "00%d", id);
    }
    if(c==3){
        sprintf(uid, "0%d", id);
    }
    if(c==4){
        sprintf(uid, "%d", id);
    }

    //write to user file
    printf("Adding new user to list\n");
    printf("Opening file descriptor\n");
    int fd = open("user_list.txt", O_RDWR | O_APPEND, 0600);
    if(fd == -1){
        perror("Cannot open file descriptor");
        exit(EXIT_FAILURE);
    }
    printf("Locking file\n");
    flock(fd, LOCK_EX);

    FILE *fp = fdopen(fd, "a");
    // FILE *fp = fopen("user_list.txt", "a");
    if(fp == NULL)
    {
        printf("Error!");
        exit(1);
    }
    // user[strlen(user)-1] = '\0';
    // pass[strlen(pass)-1] = '\0';
    // printf("%s,%s,%s", user, pass, uid);
    fprintf(fp, "%s,%s,%s\n", user, pass, uid);
    free(user);
    free(pass);

    printf("Closing file\n");
    fclose(fp);
    printf("Closing file descriptor\n");
    close(fd);
    printf("Releasing lock\n");
    flock(fd, LOCK_UN);

    //create user directory
    mkdir(uid, 0777);
    return id;
}

int login(int sockfd){
    char buf[BUFSIZ];
    char *user, *pass, *msg;
    int id = 0, len;
    memset(buf, 0, BUFSIZ);

    while(1){
        msg = "Username: ";
        write(sockfd, msg, strlen(msg));
        len = read(sockfd, buf, BUFSIZ);
        buf[len-1] = '\0'; 
        user = strdup(buf);

        msg = "Password: ";
        write(sockfd, msg, strlen(msg));
        len = read(sockfd, buf, BUFSIZ);
        buf[len-1] = '\0';
        pass = strdup(buf);
        // printf("Input %s %s\n",user, pass);

        if((id = checkLogin(user, pass)) != 0){
            break;
        }
    }

    free(user);
    free(pass);
    
    return id;
}

void menu (int sockfd, int id){
    char input[BUFSIZ], uid[6];
    char buf[BUFSIZ], *msg;
    int c=0, temp = id, menu = 0, len;
    memset(input, 0, BUFSIZ);

    printf("In Menu\n");
    
    msg = "4";
    write(sockfd, msg, strlen(msg));
    len = read(sockfd, buf, BUFSIZ); // value not needed only for blocking
    buf[len] = '\0';

    while(temp != 0){
        temp /= 10;
        c++;
    }
    if(c==1){
        sprintf(uid, "/000%d", id);
    }
    if(c==2){
        sprintf(uid, "/00%d", id);
    }
    if(c==3){
        sprintf(uid, "/0%d", id);
    }
    if(c==4){
        sprintf(uid, "/%d", id);
    }

    while(1){
        // printf("LOOP\n");
        msg = "Welcome to online storage\n1. See files\n2. Upload File\n3. Exit\nInput: ";
        write(sockfd, msg, strlen(msg));
        len = read(sockfd, buf, BUFSIZ);
        buf[len] = '\0';
        menu = atoi(buf);
        // printf("menu = %d\n", menu);
        if(menu == 1){
            // printf("FILE LIST\n");
            //display list of files
            DIR *dir;
            struct dirent *ent = NULL;
            char *dirname = strdup("./");
            strcat(dirname, uid);
            dir = opendir(dirname);
            printf("%s\n", dirname);
            free(dirname);
            if(dir == NULL){
                printf("Directory error\n");
                exit(1);
            }
            char *send = strdup("Files:\n");

            while((ent = readdir(dir)) != NULL){
                strcat(send, ent->d_name);
                strcat(send, "\n");
            }
            closedir(dir);
            write(sockfd, send, strlen(send));
            free(send);
        } else if (menu == 2){
            char *filename;
            FILE *fp;
            ssize_t len;
            int filesize, remaining = 0;


            write(sockfd, "File name: \0", 13);
            read(sockfd, input, BUFSIZ);
            filename = strdup("./");
            strcat(filename, uid);
            strcat(filename, "/");
            strcat(filename, input);
            printf("%s\n", filename);
            // filename = strdup(input);
            //upload
            recv(sockfd, input, BUFSIZ, 0);
            filesize = atoi(input);
            //fprintf(stdout, "\nFile size : %d\n", file_size);

            fp = fopen(filename, "w");
            if(fp == NULL){
                printf("File Error\n");
                exit(1);
            }

            remaining = filesize;

            while ((remaining > 0) && ((len = recv(sockfd, input, BUFSIZ, 0)) > 0)) {
                    sprintf(buf, "%ld\0", len);
                    send(sockfd, buf, BUFSIZ, 0);
                    bzero(buf, strlen(buf));

                    
                    recv(sockfd, buf, BUFSIZ, 0);
                    fprintf(stdout, "Do %s\n", buf);
                    if(atoi(buf) == 1) {
                            puts("Packet corrupted");
                            continue;
                    }
                    
                    
                    fwrite(input, sizeof(char), len, fp);
                    remaining -= len;
                    
                    printf("Received %ld bytes, %d bytes left\n", len, remaining);
            }
            fclose(fp);
            free(filename);
        } else if(menu == 3){
            exit(1);
        }
    }
    
}

// mode 1 = return 0 if not found, mode 2 = return last id if not found
// return 0 if found, return 1 if empty, return last+1 if not empty and not exist
int searchUser(char *user, int mode){
    int id = 1;
    int fd;
    FILE *fp;
    char line[BUFSIZ];
    printf("Searching for user duplicates\n");

    printf("Opening file descriptor\n");
    fd = open("user_list.txt", O_CREAT | O_RDWR | O_APPEND, 0600);
    if(fd == -1){
        perror("Cannot open file descriptor");
        exit(EXIT_FAILURE);
    }
    printf("Locking file\n");
    flock(fd, LOCK_EX);

    fp = fdopen(fd, "r");
    if (fp == NULL){
        perror("Cannot open file from file descriptor");
        exit(EXIT_FAILURE);
    }        

    char *tok = NULL;
    while(fgets(line, BUFSIZ, fp) != NULL){
        char *temp = strdup(line);
        tok = strsep(&temp, ",");
        if(strcmp(user, tok) == 0){
            printf("Duplicate found\n");
            // free(temp);
            return 0;
        }
        tok = strsep(&temp, ",");
        tok = strsep(&temp, ",");
        free(temp);
    }
    if(tok){
        id = atoi(tok) + 1;
    }

    printf("Closing file\n");
    fclose(fp);
    printf("Closing file descriptor\n");
    close(fd);
    printf("Releasing lock\n");
    flock(fd, LOCK_UN);

    return id;
}

// return 0 if not exist, return id if found
int checkLogin(char *user, char *pass){
    int id = 0;
    int fd;
    FILE *fp;
    char line[BUFSIZ];
    printf("Checking if user exists\n");

    printf("Opening file descriptor\n");
    fd = open("user_list.txt", O_CREAT | O_RDWR | O_APPEND, 0600);
    if(fd == -1){
        perror("Cannot open file descriptor");
        exit(EXIT_FAILURE);
    }
    printf("Locking file\n");
    flock(fd, LOCK_EX);

    fp = fdopen(fd, "r");
    if (fp == NULL){
        perror("Cannot open file");
        exit(EXIT_FAILURE);
    }        

    char *tok = NULL;
    while(fgets(line, BUFSIZ, fp) != NULL){
        char *temp = strdup(line);
        tok = strsep(&temp, ",");
        printf("%s %s\n", user, tok);
        if(strcmp(user, tok) == 0){
            printf("Username match\n");
            tok = strsep(&temp, ",");
            printf("%s %s\n", pass, tok);
            if(strcmp(pass, tok) == 0){
                printf("Pasword match\n");
                tok = strsep(&temp, ",");
                return atoi(tok);
            }
        }
    }
    
    printf("Closing file\n");
    fclose(fp);
    printf("Closing file descriptor\n");
    close(fd);
    printf("Releasing lock\n");
    flock(fd, LOCK_UN);

    return id;
}