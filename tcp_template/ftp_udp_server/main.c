#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <stdint.h>
#include <dirent.h>

int direxist(int sock, struct sockaddr * cli_addr, char *  path, int clilen); //смена директории
int ls(int sock, struct sockaddr * cli_addr, char * path, int clilen);  //просмотр содержимого
int pull(int sock, struct sockaddr * cli_addr, char * file, int clilen); //взять файл с сервера
int push(int sock, struct sockaddr * cli_addr, char * path, int clilen); //положить файл на сервер
int checkRec (char * mes, int counter);
int flag = 0;

void deleteShield (char * str) {
    for (unsigned int i = 0; i < strlen(str - 1); i++)
        str[i] = str[i + 1];
}

void pastShield (char * sendbuff) {
    for (int i = strlen(sendbuff)-1; i >= 0; i--)
        sendbuff[i + 1] = sendbuff[i];
    sendbuff[0] = '/';
}

void pastPacketNumber(char * post, int number, int len) {
    for (int i = len - 1; i > 1; i--) {
        post[i] = post[i-2];
    }
    post[0] = (int)(number / 10) + '0';
    post[1] = number % 10 + '0';
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
    return 1;
}

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
    char ans[3];
    memset(ans, 0, 3);
    if ((dir = opendir(path)) == NULL) {
        ans[0] = 'n'; // папки не существует, клиент не изменит адрес
    } else {
        ans[0] = 'y'; // клиент получит одобрение на изменение адреса
    }
    pastPacketNumber(ans, 1, 3);
    sendto(sock, &ans[0], 3, 0, cli_addr, clilen);
    closedir(dir);
    return 1;
}

int ls(int sock, struct sockaddr * cli_addr, char * path, int clilen) {
    DIR* dir;
    struct dirent* de;
    char ans [256];
    int sendCounter = 2;
    memset(ans, 0, 256);
    if ((dir = opendir(path)) == NULL) {
        ans[0] = 'n'; // если папки не существует
    } else {
        ans[0] = 'y';
    }
    pastPacketNumber(ans, 1, 3);
    sendto(sock, &ans[0], 3, 0, cli_addr, clilen);
    Sleep(1);
    // в цикле перебираем все содержимое, отправляя новое сообщения для каждого файла
    while ((de = readdir(dir)) != NULL) {
        memset(ans, 0, 256);
        strcat(ans, de->d_name);
        if (!(strncmp(ans, "_ls_end", 256))) pastShield(ans);
        pastPacketNumber(ans, sendCounter, 256);
        sendto(sock, &ans[0], 256, 0, cli_addr, clilen);
        sendCounter++;
    }
    memset(ans, 0, 256);
    strncat(ans, "_ls_end", 7);
    pastPacketNumber(ans, sendCounter, 256);
    sendto(sock, &ans[0], 256, 0, cli_addr, clilen); // по данной строке клиент закончит прием сообщений
    closedir(dir);
    return 1;
}

int pull(int sock, struct sockaddr * cli_addr, char * file, int clilen) {
    char temp [5];
    char sendbuff[256];
    int sendCounter = 2;
    // *передача клиенту указанного файла от сервера*
    // проверка, есть ли этот файл
    FILE * fp;
    if((fp = fopen(file, "rb")) == NULL) {
        // если нет, то сервер отправляет отказ. клиент не начинает прием
        sendbuff[0] = 'n'; //  отказ
        return 0;
    } else sendbuff[0] = 'y';
    // если да, то сервер отправляет подтверждение и начинает передачу, которую закончит спец строкой
    // подтверждение
    pastPacketNumber(sendbuff, 1, 3);
    sendto(sock, &sendbuff[0], 3, 0, cli_addr, clilen);
    // оправляем файл
    int size = 0;

    memset(sendbuff, 0, 256);
    fseek(fp, 0, SEEK_SET);
    while (!feof(fp)) {
        size = (int)fread((void *) sendbuff, sizeof(char), 254, fp);
        if (!(strncmp(sendbuff, "_end_of_file", 254))) pastShield(sendbuff);
        memset(temp, 0, 5);
        strncat(temp, makeStrFromInt(size), 5);
        pastPacketNumber(temp, sendCounter, 5);
        sendto(sock, &temp[0], 5, 0, cli_addr, clilen);
        sendCounter++;
        pastPacketNumber(sendbuff, sendCounter, 256);
        sendto(sock, &sendbuff[0], size+2, 0, cli_addr, clilen);
        sendCounter++;
    }
    char str256[5];
    memset(str256, 0, 5);
    strncat(str256, makeStrFromInt(strlen("_end_of_file")), 3);
    pastPacketNumber(str256, sendCounter, 5);
    sendto(sock, &str256[0], 5, 0, cli_addr, clilen);
    sendCounter++;
    memset(sendbuff, 0, 256);
    strncat(sendbuff, "_end_of_file", 12);
    pastPacketNumber(sendbuff, sendCounter, 256);
    sendto(sock, &sendbuff[0], 256, 0, cli_addr, clilen);
    fclose(fp);
    return 1;
}

int push(int sock, struct sockaddr * cli_addr, char * path, int clilen) {
    int size;
    char buffer [256];
    int recCounter = 2;
    // *прием сервером файла от клиента*
    // попытка открыть файл по адресу
    FILE * fp;
    if((fp = fopen(path, "wb")) == NULL) {
        // если нет, то сервер отправляет отказ. клиент не начинает прием
        buffer[0] = 'n'; //  отказ
        return 0;
    } else buffer[0] = 'y';
    // если да, то отослать подтверждение, и начать прием файла. конец приема по спец строке
    pastPacketNumber(buffer, 1, 3);
    sendto(sock, &buffer[0], 3, 0, cli_addr, clilen);

    memset(buffer, 0, 256);
    recvfrom(sock, buffer, 5, 0, cli_addr, &clilen);
    if (checkRec(buffer, recCounter)) return 0;
    recCounter++;
    size = atoi(buffer);
    memset(buffer, 0, 256);
    recvfrom(sock, buffer, size+2, 0, cli_addr, &clilen);
    if (checkRec(buffer, recCounter)) return 0;
    recCounter++;
    while (strncmp(buffer, "_end_of_file", 256)) {
        if (!(strncmp(buffer, "/_end_of_file", 256))) deleteShield(buffer);
        fwrite((void *) buffer, sizeof(char), size, fp);
        memset(buffer, 0, 256);
        recvfrom(sock, buffer, 5, 0, cli_addr, &clilen);
        if (checkRec(buffer, recCounter)) return 0;
        recCounter++;
        size = atoi(buffer);
        if (size > 254) size = 254;
        memset(buffer, 0, 256);
        recvfrom(sock, buffer, size+2, 0, cli_addr, &clilen);
        if (checkRec(buffer, recCounter)) return 0;
        recCounter++;
    }
    fflush(fp);
    fclose(fp);
    return 1;
}

int readAndWrite (int sock, struct sockaddr * cli_addr, int clilen) {
    //int sock = *((int *) temp);
    char buf[256];
    char *p = buf;
    while(1) {
        memset(buf, 0, 256);
        //int n = readn(sock, cli_addr, p, 3, clilen);
        int n = recvfrom(sock, buf, 5, 0, cli_addr, &clilen);
        if (checkRec(buf, 0)) return 3;
        if (n < 0) {
            perror("ERROR on reading");
            exit(1);
        }
        int length = atoi(p);
        memset(buf, 0, 256);
        n = recvfrom(sock, buf, length+2, 0, cli_addr, &clilen);
        if (checkRec(buf, 1)) return 3;
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
            //break;
        }
    }
    shutdown(sock, 2);
    closesocket(sock);
    return 1;
}

DWORD WINAPI closeServ(void* socket) {
    int sock = *((int *) socket);

    char buf[256];
    while (1) {
        memset(buf, 0, 256);
        fgets(buf, 256, stdin);

        if (!(strncmp(buf, "close", 5))) {
            shutdown(sock, 2);
            closesocket(sock);
            flag = 1;
            ExitThread(0);
        }
    }
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

    struct sockaddr_in serv_addr, cli_addr;

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

    CreateThread(NULL, 0, &closeServ, &sockfd, 0, NULL);
    clilen = sizeof(cli_addr);
    int check = readAndWrite(sockfd, (struct sockaddr *)&cli_addr, clilen);
    while (check == 3) {
        check = readAndWrite(sockfd, (struct sockaddr *)&cli_addr, clilen);
        if (flag == 1) break;
    }

    WSACleanup();
    return 0;
}