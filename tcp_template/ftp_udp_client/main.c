#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

static char addr [256] = "D:\\";
int parse (int sockfd, struct sockaddr * serv_addr, char * message, int len);
int dirChange (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int ls (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int pull (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int push (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int readAndWriteCycle(int sockfd, struct sockaddr * serv_addr, int len);
int checkRec (char * mes, int counter);

void makePost (char * message, char * com, char * arg) {
    strncat(message, com, strlen(com));
    strncat(message, addr, strlen(addr));
    strncat(message, arg, 256 - strlen(com) - strlen(addr));
    return;
}

void deleteShield (char * str) {
    for (unsigned int i = 0; i < strlen(str - 1); i++)
        str[i] = str[i + 1];
    return;
}

void pastShield (char * sendbuff) {
    for (int i = strlen(sendbuff)-1; i >= 0; i--)
        sendbuff[i + 1] = sendbuff[i];
    sendbuff[0] = '/';
    return;
}

int pastPacketNumber(char * message, int number, int len) {
    for (int i = len - 1; i > 1; i--) {
        message[i] = message[i-2];
    }
    message[0] = (int)(number / 10) + '0';
    message[1] = number % 10 + '0';
    return 1;
}

void makeStrFromInt (int len, char * p) {
    memset(p, 0, 3);
    int temp;
    p[2] = ((int)(len % 10) + '0');
    temp = len / 10;
    p[1] = ((int)(temp % 10) + '0');
    p[0] = ((int)(temp / 10) + '0');
}

int checkRec (char * mes, int counter) {
    char temp[2];
    temp[0] = mes[0];
    temp[1] = mes[1];
    for (int i = 0; i < strlen(mes); i++) {
        mes[i] = mes[i+2];
    }
    int co = atoi(temp);
    if (co == counter + 1) return 0;
    printf("Ошибка приема - неверный порядок пакетов\n");
    return 0;
}

int ls (int sockfd, struct sockaddr * serv_addr, char * arg, int len) {
    char post[256];

    int recConter = 0;
    bzero(post, 256);
    makePost(post, "ls ", arg);
    pastPacketNumber(post, 2, 256);
    char str256[5];
    bzero(str256, 5);
    char p[3];
    makeStrFromInt(strlen(post), p);
    strncat(str256, p, 3);
    pastPacketNumber(str256, 1, 5);
    sendto(sockfd, &str256[0], strlen(str256), 0, serv_addr, len);
    sendto(sockfd, &post[0], strlen(post), 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    bzero(post, 256);
    recvfrom(sockfd, post, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if(checkRec(post, recConter)) return 0;
    recConter++;
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        recvfrom(sockfd, post, 256, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(post, recConter)) return 0;
        recConter++;
        while (strncmp(post, "_ls_end", 7)) {
            if(!(strncmp(post, "/_ls_end", 8))) deleteShield(post);
            printf("%s", post);
            printf("\n");
            memset(post, 0, 256);
            recvfrom(sockfd, post, 256, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(post, recConter)) return 0;
            recConter++;
        }
        return 1;
    }
    return 0;
}

int pull (int sockfd, struct sockaddr * serv_addr, char * arg, int len) {
    int size;
    FILE * fp;
    if((fp = fopen(arg, "wb")) == NULL) {
        printf("Can't create file \n");
        return 0;
    }
    int recCounter = 0;
    int sendCounter = 3;
    char post[256];
    bzero(post, 256);
    makePost(post, "pull ", arg);
    pastPacketNumber(post, 2, 256);
    char str256[5];
    bzero(str256, 5);
    char p[3];
    makeStrFromInt(strlen(post), p);
    strncat(str256, p, 3);
    pastPacketNumber(str256, 1, 5);
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], strlen(post), 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    bzero(post, 256);
    recvfrom(sockfd, post, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if(checkRec(post, recCounter)) return 0;
    recCounter++;
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        recvfrom(sockfd, post, 5, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(post, recCounter)) return 0;
        recCounter++;
        size = atoi(post);
        memset(post, 0, 256);
        recvfrom(sockfd, post, size+2, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(post, recCounter)) return 0;
        recCounter++;
        while (strncmp(post, "_end_of_file", 256)) {
            if (!(strncmp(post, "/_end_of_file", 256))) deleteShield(post);
            fwrite((void *) post, sizeof(char), size, fp);
            memset(post, 0, 256);
            recvfrom(sockfd, post, 5, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(post, recCounter)) return 0;
            recCounter++;
            size = atoi(post);
            if (size > 254) size = 254;
            memset(post, 0, 256);
            recvfrom(sockfd, post, size+2, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(post, recCounter)) return 0;
            recCounter++;
        }
        fflush(fp);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

int push (int sockfd, struct sockaddr * serv_addr, char * arg, int len) {
    FILE * fp;
    if((fp = fopen(arg, "rb")) == NULL) {
        printf("Can't read file \n");
        return 0;
    }
    char post[256];
    int recCounter = 0;
    int sendCounter = 3;
    char temp[5];
    bzero(post, 256);
    makePost(post, "push ", arg);
    pastPacketNumber(post, 2, 256);
    char str256[5];
    char p[3];
    makeStrFromInt(strlen(post), p);
    strncat(str256, p, 3);
    pastPacketNumber(str256, 1, 5);
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], strlen(post), 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    bzero(post, 256);
    recvfrom(sockfd, post, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if (checkRec(post, recCounter)) return 0;
    int size = 0;
    if (!(strncmp(post, "y", 1))) {
        memset(post, 0, 256);
        fseek(fp, 0, SEEK_SET);
        while (!feof(fp)) {
            size = (int)fread((void *) post, sizeof(char), 254, fp);
            if (!(strncmp(post, "_end_of_file", 254))) pastShield(post);
            memset(temp, 0, 5);
            makeStrFromInt(size, p);
            strncat(temp, p, 3);
            pastPacketNumber(temp, sendCounter, 5);
            sendto(sockfd, &temp[0], 5, 0, serv_addr, len);
            sendCounter++;
            pastPacketNumber(post, sendCounter, 256);
            sendto(sockfd, &post[0], size+2, 0, serv_addr, len);
            sendCounter++;
        }
        memset(str256, 0, 5);
        makeStrFromInt(strlen("_end_of_file"), p);
        strncat(str256, p, 3);
        pastPacketNumber(str256, sendCounter, 5);
        sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
        sendCounter++;
        char strEnd[256];
        memset(strEnd, 0, 256);
        strcat(strEnd, "_end_of_file");
        pastPacketNumber(strEnd, sendCounter, 256);
        sendto(sockfd, &strEnd[0], 256, 0, serv_addr, len);
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

int dirChange(int sockfd, struct sockaddr * serv_addr, char * arg, int len){

    char post[256];
    bzero(post, 256);
    int recConter = 0;
    makePost(post, "cd ", arg);
    pastPacketNumber(post, 2, 256);
    char str256[5];
    bzero(str256, 5);
    char p[3];
    makeStrFromInt(strlen(post), p);
    strncat(str256, p, 3);
    pastPacketNumber(str256, 1, 5);
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], strlen(post), 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    bzero(str256, 256);
    recvfrom(sockfd, post, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if (checkRec(post, recConter)) return 1;
    if (!(strncmp(post, "y", 1))) {
        if (!(strncmp(arg, "..", 2))) {
            int i, f = 0;
            for(i = 255; i > 0; i--) {
                if (addr[i] == '\\') {
                    f++;
                    if (f == 2) {
                        break;
                    }
                }
                addr[i] = '\0';
            }
            addr[0] = 'D';
            addr[1] = ':';
            addr[2] = '\\';
        } else {
            strcat(addr, arg);
            strcat(addr, "\\");
        }
        return 0;
    }
    return 1;
}

int readAndWriteCycle (int sockfd, struct sockaddr * serv_addr, int len) {
    char buffer[256];
    sendto(sockfd, "addr", 4, 0, serv_addr, len);
    recvfrom(sockfd, addr, 256, 0, serv_addr, &len);
    while (1) {
        int i;
        memset(buffer, 0, 256);
        printf("%s>", addr);
        fgets(buffer, 254, stdin);
        for (i = 0; i < 254; i ++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';
                break;
            }
        }
        int n = parse(sockfd, serv_addr, buffer , len);
        if (n == 1) break;
        if (n == 2) printf("Unknown command\n");
    }
    return 0;
}

int parse(int sockfd, struct sockaddr * serv_addr, char * message, int len) {
    char command [6]; // храним команду
    char arg [248]; // храним аргументы после команды
    memset (command, 0, 6);
    memset (arg, 0, 248);

    int i = 0;
    int j = 0;
    // вытащим команду из строки
    for (; ((message[i] != ' ') && (i < strlen(message))); i++) command[i] = message[i];
    i++;
    // сохраним аргументы из оставшейся части строки
    for (; i < strlen(message); i++) arg[j++] = message[i];

    if (!(strncmp(command, "exit", 4))) {
        char str4[5] = "01004";
        for (int i = 6; i > 1; i--) {
            command[i] = command[i-2];
        }
        command[0] = '0';
        command[1] = '2';
        sendto(sockfd, &str4[0], 5, 0, serv_addr, len);
        sendto(sockfd, &command[0], 6, 0, serv_addr, len);
        return 1;
    } else if(!(strncmp(command, "cd", 2))) dirChange (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "ls", 2))) ls (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "pull", 4))) pull (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "push", 4))) push (sockfd, serv_addr, arg, len);
    else return 2;
    return 0;
}

int main(int argc, char *argv[]) {

    int sockfd, n;
    uint16_t portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    char *p = buffer;


    portno = (uint16_t) 5001;
    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    server = gethostbyname("192.168.56.1");

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

    int len = sizeof(serv_addr);
    readAndWriteCycle(sockfd, (struct sockaddr *)&serv_addr, len);

    close(sockfd);

    return 0;
}