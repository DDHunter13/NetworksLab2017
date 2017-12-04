#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

static char addr [256] = "D:\\";
int parse (int sockfd, struct sockaddr * serv_addr, char * message, int len);
int addrPaste (char * message);
int addrChange (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int ls (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int pull (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int push (int sockfd, struct sockaddr * serv_addr, char * arg, int len);
int readn (int sockfd, struct sockaddr * serv_addr, char*, int n, int len);
int readAndWriteCycle(int sockfd, struct sockaddr * serv_addr, int len);
int checkRec (char * mes, int counter);

int addrPaste (char * message) {
    char str[256];
    memset(str, 0 ,256);
    strcat(str, addr);
    strcat(str, message);
    message = str;
    return 1;
}

int ls (int sockfd, struct sockaddr * serv_addr, char * arg, int len) {
    //addrPaste(arg);
    char post[256];
    char mes[256];
    int recConter = 0;
    int sendCounter = 2;
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "02ls ");
    strcat(post, addr);
    strcat(post, arg);
    char str256[5] = "01256";
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], 256, 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    recvfrom(sockfd, mes, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if(checkRec(mes, recConter)) return 0;
    recConter++;
    if (!(strncmp(mes, "y", 1))) {
        memset(mes, 0, 256);
        recvfrom(sockfd, mes, 256, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(mes, recConter)) return 0;
        recConter++;
        while (strncmp(mes, "_ls_end", 7)) {
            printf("%s", mes);
            printf("\n");
            memset(mes, 0, 256);
            recvfrom(sockfd, mes, 256, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(mes, recConter)) return 0;
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
    int sendCounter = 2;
    //addrPaste(arg);
    char post[256];
    char mes[256];
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "02pull ");
    strcat(post, addr);
    strcat(post, arg);
    char str256[5] = "01256";
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], 256, 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    recvfrom(sockfd, mes, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if(checkRec(mes, recCounter)) return 0;
    recCounter++;
    if (!(strncmp(mes, "y", 1))) {
        memset(mes, 0, 256);
        recvfrom(sockfd, mes, 5, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(mes, recCounter)) return 0;
        recCounter++;
        size = atoi(mes);
        memset(mes, 0, 256);
        recvfrom(sockfd, mes, size+2, 0, (struct sockaddr *)&serv_addr2, &length);
        if(checkRec(mes, recCounter)) return 0;
        recCounter++;
        while (strncmp(mes, "_end_of_file", 256)) {
            if (!(strncmp(mes, "/_end_of_file", 256))) {
                int i;
                for (i = 0; i < strlen(mes) - 1; i++)
                    mes[i] = mes[i + 1];
            }
            fwrite((void *) mes, sizeof(char), size, fp);
            fflush(fp);
            memset(mes, 0, 256);
            recvfrom(sockfd, mes, 5, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(mes, recCounter)) return 0;
            recCounter++;
            size = atoi(mes);
            if (size > 254) size = 254;
            memset(mes, 0, 256);
            recvfrom(sockfd, mes, size+2, 0, (struct sockaddr *)&serv_addr2, &length);
            if(checkRec(mes, recCounter)) return 0;
            recCounter++;
        }
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
    //addrPaste(arg);
    char post[256];
    char mes[256];
    int recCounter = 0;
    int sendCounter = 3;
    char temp[5];
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "02push ");
    strcat(post, addr);
    strcat(post, arg);
    char str256[5] = "01256";
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], 256, 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    recvfrom(sockfd, mes, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if (checkRec(mes, recCounter)) return 0;
    recCounter++;
    int size = 0;
    if (!(strncmp(mes, "y", 1))) {
        memset(mes, 0, 256);
        fseek(fp, 0, SEEK_SET);
        while (!feof(fp)) {
            size = (int)fread((void *) mes, sizeof(char), 254, fp);
            if (!(strncmp(mes, "_end_of_file", 254))) {
                int i;
                for (i = strlen(mes)-1; i >= 0; i--)
                    mes[i + 1] = mes[i];
                mes[0] = '/';
            }
            memset(temp, 0, 3);
            int temp2;
            temp[0] = (int)(sendCounter / 10) + '0';
            temp[1] = sendCounter % 10 + '0';
            temp[4] = ((int)(size % 10) + '0');
            temp2 = size / 10;
            temp[3] = ((int)(temp2 % 10) + '0');
            temp[2] = ((int)(temp2 / 10) + '0');
            sendto(sockfd, &temp[0], 5, 0, serv_addr, len);
            sendCounter++;
            for (int i = 256; i > 1; i--) {
                mes[i] = mes[i-2];
            }
            mes[0] = (int)(sendCounter / 10) + '0';
            mes[1] = sendCounter % 10 + '0';
            sendto(sockfd, &mes[0], size+2, 0, serv_addr, len);
            sendCounter++;
        }
        memset(str256, 0, 5);
        str256[0] = (int)(sendCounter / 10) + '0';
        str256[1] = sendCounter % 10 + '0';
        strcat(str256, "256");
        sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
        sendCounter++;
        char strEnd[256];
        memset(strEnd, 0, 256);
        strEnd[0] = (int)(sendCounter / 10) + '0';
        strEnd[1] = sendCounter % 10 + '0';
        strcat(strEnd, "_end_of_file");
        sendto(sockfd, &strEnd[0], 256, 0, serv_addr, len);
        sendCounter++;
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

int addrChange(int sockfd, struct sockaddr * serv_addr, char * arg, int len){
    //addrPaste(arg);
    char post[256];
    char mes[256];
    int recConter = 0;
    int sendCounter = 2;
    memset(post, 0, 256);
    memset(mes, 0, 256);
    strcat(post, "02cd ");
    strcat(post, addr);
    strcat(post, arg);
    char str256[5] = "01256";
    sendto(sockfd, &str256[0], 5, 0, serv_addr, len);
    sendto(sockfd, &post[0], 256, 0, serv_addr, len);
    struct sockaddr_in serv_addr2;
    int length = sizeof(serv_addr2);
    recvfrom(sockfd, mes, 3, 0, (struct sockaddr *)&serv_addr2, &length);
    if (checkRec(mes, recConter)) return 1;
    recConter++;
    if (!(strncmp(mes, "y", 1))) {
        //memset(addr, 0, 256);
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
    } else if(!(strncmp(command, "cd", 2))) addrChange (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "ls", 2))) ls (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "pull", 4))) pull (sockfd, serv_addr, arg, len);
    else if(!(strncmp(command, "push", 4))) push (sockfd, serv_addr, arg, len);
    else return 2;
    return 0;
}

int readn(int sockfd, struct sockaddr * serv_addr, char *buf, int n, int len){
    int k;
    int off = 0;
    for(int i = 0; i < n; ++i){
        k = recvfrom(sockfd, buf + off, 1, 0, serv_addr, &len);
        off += 1;
        if (k < 0){
            printf("Error reading from socket \n");
            exit(1);
        }
    }
    return 0;
}

int checkRec (char * mes, int counter) {
    char temp[2];
    temp[0] = mes[0];
    temp[1] = mes[1];
    for (int i = 0; i < 254; i++) {
        mes[i] = mes[i+2];
    }
    int co = atoi(temp);
    if (co == counter + 1) return 0;
    printf("Ошибка приема - неверный порядок пакетов\n");
    return 1;
}

int main(int argc, char *argv[]) {

    int sockfd, n;
    uint16_t portno;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    char *p = buffer;

    //if (argc < 3) {
    //    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    //    exit(0);
    //}

    //portno = (uint16_t) atoi(argv[2]);
    portno = (uint16_t) 5001;
    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    //server = gethostbyname(argv[1]);
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

    close(sockfd);

    return 0;
}