#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <stdint.h>
#include <dirent.h>

int direxist(int sock, struct sockaddr * cli_addr, char *  path, int clilen); //смена директории
int ls(int sock, struct sockaddr * cli_addr, char * path, int clilen);  //просмотр содержимого
int pull(int sock, struct sockaddr * cli_addr, char * file, int clilen); //взять файл с сервера
int push(int sock, struct sockaddr * cli_addr, char * path, int clilen); //положить файл на сервер
int readn(int sockfd, struct sockaddr * cli_addr, char *buf, int n, int clilen);

int parse (int sock, struct sockaddr * cli_addr, char * message, int clilen) {
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

    if (!(strncmp(command, "cd", 2))) direxist(sock, cli_addr, arg, clilen);
    else
    if (!(strncmp(command, "ls", 2))) ls(sock, cli_addr, arg, clilen);
    else
    if (!(strncmp(command, "pull", 4))) pull(sock, cli_addr, arg, clilen);
    else
    if (!(strncmp(command, "push", 4))) push (sock, cli_addr, arg, clilen);
    else
    if (!(strncmp(command, "exit", 4))) return 2;
    else return 0;
    return 1;
}

int direxist(int sock, struct sockaddr * cli_addr, char *  path, int clilen) {
    DIR* dir;
    if ((dir = opendir(path)) == NULL) {
        char n[1] = "n";
        sendto(sock, &n[0], 1, 0, cli_addr, clilen); // папки не существует, клиент не изменит адрес
    } else {
        char y[1] = "y";
        sendto(sock, &y[0], 1, 0, cli_addr, clilen); // клиент получит одобрение на изменение адреса
    }
    closedir(dir);
    return 1;
}

int ls(int sock, struct sockaddr * cli_addr, char * path, int clilen) {
    DIR* dir;
    struct dirent* de;
    char ans [256];
    memset(ans, 0, 256);
    if ((dir = opendir(path)) == NULL) {
        char n[1] = "n";
        sendto(sock, &n[0], 1, 0, cli_addr, clilen); // если папки не существует
    } else {
        char y[1] = "y";
        sendto(sock, &y[0], 1, 0, cli_addr, clilen);
    }
    // в цикле перебираем все содержимое, отправляя новое сообщения для каждого файла
    while ((de = readdir(dir)) != NULL) {
        memset(ans, 0, 256);
        strcat(ans, de->d_name);
        if (!(strncmp(ans, "_ls_end", 256))) {
            int i;
            for (i = strlen(ans)-1; i >= 0; i--)
                ans[i + 1] = ans[i];
            ans[0] = '/';
        }
        sendto(sock, &ans[0], 256, 0, cli_addr, clilen);
    }
    char lsEnd[256] = "_ls_end";
    sendto(sock, &lsEnd[0], 256, 0, cli_addr, clilen); // по данной строке клиент закончит прием сообщений
    closedir(dir);
    return 1;
}

int pull(int sock, struct sockaddr * cli_addr, char * file, int clilen) {
    char temp [3];
    // *передача клиенту указанного файла от сервера*
    // проверка, есть ли этот файл
    FILE * fp;
    if((fp = fopen(file, "rb")) == NULL) {
        // если нет, то сервер отправляет отказ. клиент не начинает прием
        char n[1] = "n";
        sendto(sock, &n[0], 1, 0, cli_addr, clilen); //  отказ
        return 0;
    }
    // если да, то сервер отправляет подтверждение и начинает передачу, которую закончит спец строкой
    // подтверждение
    char y[1] = "y";
    sendto(sock, &y[0], 1, 0, cli_addr, clilen);
    // оправляем файл
    int size = 0;
    char sendbuff[256];
    memset(sendbuff, 0, 256);
    fseek(fp, 0, SEEK_SET);
    while (!feof(fp)) {
        size = (int)fread((void *) sendbuff, sizeof(char), 256, fp);
        if (!(strncmp(sendbuff, "_end_of_file", 256))) {
            int i;
            for (i = strlen(sendbuff)-1; i >= 0; i--)
                sendbuff[i + 1] = sendbuff[i];
            sendbuff[0] = '/';
        }
        memset(temp, 0, 3);
        int temp2;
        temp[2] = ((int)(size % 10) + '0');
        temp2 = size / 10;
        temp[1] = ((int)(temp2 % 10) + '0');
        temp[0] = ((int)(temp2 / 10) + '0');
        sendto(sock, &temp[0], 3, 0, cli_addr, clilen);
        sendto(sock, &sendbuff[0], size, 0, cli_addr, clilen);
    }
    char str256[3] = "256";
    sendto(sock, &str256[0], 3, 0, cli_addr, clilen);
    char endFile[256] = "_end_of_file";
    sendto(sock, &endFile[0], 256, 0, cli_addr, clilen);
    fclose(fp);
    return 1;
}

int push(int sock, struct sockaddr * cli_addr, char * path, int clilen) {
    int size;
    // *прием сервером файла от клиента*
    // попытка открыть файл по адресу
    FILE * fp;
    if((fp = fopen(path, "wb")) == NULL) {
        // если нет, то отослать отказ. клиент не начинает передачу
        char n[1] = "n";
        sendto(sock, &n[0], 1, 0, cli_addr, clilen);
        return 0;
    }
    // если да, то отослать подтверждение, и начать прием файла. конец приема по спец строке
    char y[1] = "y";
    sendto(sock, &y[0], 1, 0, cli_addr, clilen);
    char buffer [256];
    memset(buffer, 0, 256);
    recvfrom(sock, buffer, 3, 0, cli_addr, &clilen);
    size = atoi(buffer);
    memset(buffer, 0, 256);
    recvfrom(sock, buffer, size, 0, cli_addr, &clilen);
    while (strncmp(buffer, "_end_of_file", 256)) {
        if (!(strncmp(buffer, "/_end_of_file", 256))) {
            int i;
            for (i = 0; i < strlen(buffer) - 1; i++)
                buffer[i] = buffer[i + 1];
        }
        fwrite((void *) buffer, sizeof(char), size, fp);
        fflush(fp);
        memset(buffer, 0, 256);
        recvfrom(sock, buffer, 3, 0, cli_addr, &clilen);
        size = atoi(buffer);
        memset(buffer, 0, 256);
        recvfrom(sock, buffer, size, 0, cli_addr, &clilen);
    }
    fclose(fp);
    return 1;
}


int readn(int sockfd, struct sockaddr * cli_addr, char *buf, int n, int clilen) {
    int k;
    int off = 0;
    for(int i = 0; i < n; ++i) {
        k = recvfrom(sockfd, buf + off, 1, 0, cli_addr, &clilen);
        off += 1;
        if (k < 0) {
            printf("Error reading from socket \n");
            exit(1);
        }
    }
    return off;
}

int readAndWrite (int sock, struct sockaddr * cli_addr, int clilen) {
    //int sock = *((int *) temp);
    char buf[256];
    char *p = buf;
    while(1) {
        memset(buf, 0, 256);
        //int n = readn(sock, cli_addr, p, 3, clilen);
        int n = recvfrom(sock, buf, 3, 0, cli_addr, &clilen);
        if (n < 0) {
            perror("ERROR on reading");
            exit(1);
        }
        int length = atoi(p);
        memset(buf, 0, 256);
        n = recvfrom(sock, buf, length, 0, cli_addr, &clilen);
        if (n < 0) {
            perror("ERROR on reading");
            exit(1);
        }
        n = parse(sock, cli_addr, buf, clilen);
        if (n == 0) {
            char unk[256] = "Unknown command";
            sendto(sock, &unk[0], 256, 0, cli_addr, clilen);
        }
        if (n == 2) {
            break;
        }
    }
    shutdown(sock, 2);
    closesocket(sock);
    return 1;
}

int main(int argc, char *argv[]) {

    WSADATA wsaData;

    unsigned int t;
    t = WSAStartup(MAKEWORD(2,2), &wsaData);

    if (t != 0) {
        printf("WSAStartup failed: %ui\n", t);
        return 1;
    }

    int sockfd;
    uint16_t portno;
    int clilen;
    char buffer[256];
    //char *p = buffer;
    struct sockaddr_in serv_addr, cli_addr;
    //ssize_t n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    portno = 5001;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }
    //Sleep (5000);

    clilen = sizeof(cli_addr);
    readAndWrite(sockfd, (struct sockaddr *)&cli_addr, clilen);

    WSACleanup();
    return 0;
}