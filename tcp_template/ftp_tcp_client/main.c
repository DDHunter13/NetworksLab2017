#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <stdint.h>
#include <string.h>

static char addr [256] = "/home/user/";
int parse (int sockfd, char * message);
int addrPaste (char * message);
int addrChange (int sockfd, char * arg);
int ls (int sockfd, char * arg);
int pull (int sockfd, char * arg);
int push (int sockfd, char * arg);
int readn (int, char*, int);
int readAndWriteCycle(int sockfd);

int addrPaste (char * message) {
    char str[256];
    memset(str, 0 ,256);
    strcat(str, addr);
    strcat(str, message);
    message = str;
    return 0;
}

int ls (int sockfd, char * arg) {
    addrPaste(arg);
    char * post[256];
    char * mes[256];
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "ls ");
    strcat(post, arg);
    send(sockfd, "256", 3, 0);
    send(sockfd, post, 256, 0);
    readn(sockfd, mes, 256);
    if (strncmp(mes, "y", 1)) {
        memset(mes, 0, 256);
        readn(sockfd, mes, 256);
        while (!(strncmp(mes, "_ls_end", 7))) {
            printf(mes);
            printf("\n");
            memset(mes, 0, 256);
            readn(sockfd, mes, 256);
        }
        return 0;
    }
    return 1;
}

int pull (int sockfd, char * arg) {

}
int push (int sockfd, char * arg) {

}

int addrChange(int sockfd, char * arg){
    addrPaste(arg);
    char * post[256];
    char * mes[256];
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "cd ");
    strcat(post, arg);
    send(sockfd, "256", 3, 0);
    send(sockfd, post, 256, 0);
    readn(sockfd, mes, 256);
    if (strncmp(mes, "y", 1)) {
        memset(addr, 0, 256);
        strcat(addr, arg);
        strcat(addr, "/");
        return 0;
    }
    return 1;
}

int readAndWriteCycle (int sockfd) {
    char buffer[256];
    while (1) {
        memset(buffer, 0, 256);
        fgets(buffer, 255, stdin);
        int n = parse(sockfd, buffer);
        if (n == 1) break;
        if (n == 2) printf("Unknown command\n");
    }
    return 0;
}

int parse(int sockfd, char * message) {
    char command [6]; // храним команду
    char arg [250]; // храним аргументы после команды
    memset (command, 0, 6);
    memset (arg, 0, 250);

    int i = 0;
    int j = 0;
    // вытащим команду из строки
    for (; ((message[i] != ' ') && (i < strlen(message))); i++) command[i] = message[i];
    i++;
    // сохраним аргументы из оставшейся части строки
    for (; i < strlen(message); i++) arg[j++] = message[i];

    if (strncmp(command, "exit", 4)) {
        send(sockfd, "004", 3, 0);
        send(sockfd, command, 4, 0);
        return 1;
    } else if(strncmp(command, "cd", 2)) addrChange (sockfd, arg);
        else if(strncmp(command, "ls", 2)) ls (sockfd, arg);
            else if(strncmp(command, "pull", 4)) pull (sockfd, arg);
                else if(strncmp(command, "push", 4)) push (sockfd, arg);
                    else return 2;
    return 0;
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

int main(int argc, char *argv[]) {

    WSADATA wsaData;

    unsigned int t;
    t = WSAStartup(MAKEWORD(2,2), &wsaData);

    if (t != 0) {
        printf("WSAStartup failed: %ui\n", t);
        return 1;
    }

    int sockfd, n;
    uint16_t portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    char *p = buffer;

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = (uint16_t) atoi(argv[2]);

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname(argv[1]);

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
    /* Now ask for a message from the user, this message
       * will be read by server
    */

    /*printf("Please enter the message: ");
    memset(buffer, 0, 256);
    fgets(buffer, 255, stdin);


    n = send(sockfd, buffer, 255, 0);

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }


    memset(buffer, 0, 256);
    readn(sockfd, p, 255);

    printf("%s\n", buffer);*/

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
