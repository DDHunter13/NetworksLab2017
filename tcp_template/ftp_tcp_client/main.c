#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <stdint.h>
#include <string.h>

#define UNKNOWN_COMMAND 2
#define BREAK 1

static char addr [256] = "";
int parse (int sockfd, char * message);
int dirChange (int sockfd, char * arg);
int ls (int sockfd, char * arg);
int pull (int sockfd, char * arg);
int push (int sockfd, char * arg);
int readn (int, char*, int);
int readAndWriteCycle(int sockfd);
int makePost(char * post, char * com, char * arg);

int makePost(char * post, char * com, char * arg) {
    memset(post, 0, 256);
    strncat(post, com, strlen(com));
    strncat(post, addr, strlen(addr));
    strncat(post, arg, 256 - strlen(com) - strlen(arg));
    return 1;
}

void deleteShield (char * str) {
    for (unsigned int i = 0; i < strnlen(str, 256) - 1; i++)
        str[i] = str[i + 1];
}

void pastShield (char * sendbuff) {
    for (int i = strnlen(sendbuff, 256) - 1; i >= 0; i--)
        sendbuff[i + 1] = sendbuff[i];
    sendbuff[0] = '/';
}

char * makeStrFromInt (int len) {
    char buf[3];
    char * p = buf;
    memset(p, 0, 3);
    int temp;
    p[2] = ((int)(len % 10) + '0');
    temp = len / 10;
    p[1] = ((int)(temp % 10) + '0');
    p[0] = ((int)(temp / 10) + '0');
    return p;
}

int ls (int sockfd, char * arg) {
    char post[256];
    makePost(post, "ls ", arg);
    send(sockfd, makeStrFromInt(strlen(post)), 3, 0);
    send(sockfd, post, strlen(post), 0);
    memset(post, 0 ,256);
    readn(sockfd, post, 1);
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        readn(sockfd, post, 256);
        while (strncmp(post, "_ls_end", 7)) {
            if (!(strncmp(post, "/_ls_end", 8))) deleteShield(post);
            printf(post);
            printf("\n");
            memset(post, 0, 256);
            readn(sockfd, post, 256);
        }
        return 1;
    }
    return 0;
}

int pull (int sockfd, char * arg) {
    int size;
    FILE * fp;
    if((fp = fopen(arg, "wb")) == NULL) {
        printf("Can't create file \n");
        return 0;
    }
    char post[256];
    makePost(post, "pull ", arg);
    send(sockfd, makeStrFromInt(strlen(post)), 3, 0);
    send(sockfd, post, strlen(post), 0);
    memset(post, 0 , 256);
    readn(sockfd, post, 1);
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        readn(sockfd, post, 3);
        size = atoi(post);
        memset(post, 0, 256);
        readn(sockfd, post, size);
        while (strncmp(post, "_end_of_file", 256)) {
            if (!(strncmp(post, "/_end_of_file", 256))) deleteShield(post);
            fwrite((void *) post, sizeof(char), size, fp);
            memset(post, 0, 256);
            readn(sockfd, post, 3);
            size = atoi(post);
            memset(post, 0, 256);
            readn(sockfd, post, size);
        }
        fflush(fp);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

int push (int sockfd, char * arg) {
    FILE * fp;
    if((fp = fopen(arg, "rb")) == NULL) {
        printf("Can't read file \n");
        return 0;
    }
    char post[256];
    makePost(post, "push ", arg);
    send(sockfd, makeStrFromInt(strlen(post)), 3, 0);
    send(sockfd, post, 256, 0);
    memset(post, 0 , 256);
    readn(sockfd, post, 1);
    int size = 0;
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        fseek(fp, 0, SEEK_SET);
        while (!feof(fp)) {
            size = (int)fread((void *) post, sizeof(char), 256, fp);
            if (!(strncmp(post, "_end_of_file", 256))) pastShield(post);
            send(sockfd, makeStrFromInt(size), 3, 0);
            send(sockfd, post, size, 0);
        }
        send(sockfd, makeStrFromInt(strlen("_end_of_file")), 3, 0);
        send(sockfd, "_end_of_file", strlen("_end_of_file"), 0);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

int dirChange(int sockfd, char * arg){
    char post[256];
    makePost(post, "cd ", arg);
    send(sockfd, makeStrFromInt(strlen(post)), 3, 0);
    send(sockfd, post, strlen(post), 0);
    memset(post, 0, 256);
    readn(sockfd, post, 1);
    if (!(strncmp(post, "y", 1))) {
        //memset(addr, 0, 256);
        if (!(strncmp(arg, "..", 2))) {
            int i, f = 0;
            for(i = 255; i > 0; i--) {
                if (addr[i] == '/') {
                    f++;
                    if (f == 2) {
                        break;
                    }
                }
                addr[i] = '\0';
            }
            addr[0] = '/';
        } else {
            strcat(addr, arg);
            strcat(addr, "/");
        }
        return 0;
    }
    return 1;
}

int readAndWriteCycle (int sockfd) {
    char buffer[256];
    recv(sockfd, addr, 256, 0);
    while (1) {
        int i;
        memset(buffer, 0, 256);
        printf("%s>", addr);
        fgets(buffer, 255, stdin);
        for (i = 0; i < 256; i ++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
                break;
            }
        }
        int n = parse(sockfd, buffer);
        if (n == BREAK) break;
        if (n == UNKNOWN_COMMAND) printf("Unknown command\n");
    }
    return 0;
}

int parse(int sockfd, char * message) {
    char command [6]; // храним команду
    char arg [250]; // храним аргументы после команды
    memset (command, 0, 6);
    memset (arg, 0, 250);

    unsigned int i = 0;
    int j = 0;
    // вытащим команду из строки
    for (; ((message[i] != ' ') && (i < strlen(message))); i++) command[i] = message[i];
    i++;
    // сохраним аргументы из оставшейся части строки
    for (; i < strlen(message); i++) arg[j++] = message[i];

    if (!(strncmp(command, "exit", 4))) {
        send(sockfd, "004", 3, 0);
        send(sockfd, command, 4, 0);
        return BREAK;
    }
    if(!(strncmp(command, "cd", 2))) {dirChange (sockfd, arg); return 0;}
    if(!(strncmp(command, "ls", 2))) {ls (sockfd, arg); return 0;}
    if(!(strncmp(command, "pull", 4))) {pull (sockfd, arg); return 0;}
    if(!(strncmp(command, "push", 4))) {push (sockfd, arg); return 0;};
    return UNKNOWN_COMMAND;
}

int readn(int sockfd, char *buf, int n){
    int k;
    int off = 0;
    for(int i = 0; i < n; ++i){
        k = recv(sockfd, buf + off, 1, 0);
        off += 1;
        if (k < 0){
            printf("Error reading from socket \n");
            exit(1);
        }
    }
    return 0;
}

int main(/*int argc, char *argv[]*/) {

    WSADATA wsaData;

    int t;
    t = WSAStartup(MAKEWORD(2,2), &wsaData);

    if (t != 0) {
        printf("WSAStartup failed: %ui\n", t);
        return 1;
    }

    int sockfd;
    uint16_t portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = (uint16_t) 5001;
            /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    //server = gethostbyname(argv[1]);
    server = gethostbyname("192.168.56.101");

    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *) &serv_addr.sin_addr.s_addr, server->h_addr, (size_t) server->h_length);
    serv_addr.sin_port = htons(portno);

    /* Now connect to the server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }


    readAndWriteCycle(sockfd);

    t = shutdown(sockfd, SD_BOTH);
    if (t == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }
    closesocket(sockfd);
    WSACleanup();

    return 0;
}