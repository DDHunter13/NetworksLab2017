#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>

static char startDir [256] = "/home/user/";

int direxist(int sock, char *  path); //смена директории
int ls(int sock, char * path);  //просмотр содержимого
int pull(int sock, char * file); //взять файл с сервера
int push(int sock, char * path); //положить файл на сервер
int readn(int sockfd, char *buf, int n);

int sockets[100];
int flags[100];

char * makeStrFromInt (int len) {
    char buf[3];
    char * p = buf;
    int temp;
    p[2] = ((int)(len % 10) + '0');
    temp = len / 10;
    p[1] = ((int)(temp % 10) + '0');
    p[0] = ((int)(temp / 10) + '0');
    return p;
}

void deleteShield (char * str) {
    for (int i = 0; i < strnlen(str, 256) - 1; i++)
        str[i] = str[i + 1];
}

void pastShield (char * sendbuff) {
    for (int i = strnlen(sendbuff, 256) - 1; i >= 0; i--)
        sendbuff[i + 1] = sendbuff[i];
    sendbuff[0] = '/';
}

int parse (int sock, char * message) {
    char command [6]; // храним команду
    char arg [250]; // храним аргументы после команды
    bzero (command, 6);
    bzero (arg, 250);

    int i = 0;
    int j = 0;
    // вытащим команду из строки
    for (; ((message[i] != ' ') && (i < strnlen(message, 256))); i++) command[i] = message[i];
    i++;
    // сохраним аргументы из оставшейся части строки
    for (; i < strnlen(message, 256); i++) arg[j++] = message[i];

    if (!(strncmp(command, "cd", 2))) direxist(sock, arg);

    if (!(strncmp(command, "ls", 2))) ls(sock, arg);

    if (!(strncmp(command, "pull", 4))) pull(sock, arg);

    if (!(strncmp(command, "push", 4))) push (sock,arg);

    if (!(strncmp(command, "exit", 4))) return 2;
    return 1;
}

int direxist(int sock, char *  path) {
    DIR* dir;
    char ans [1];
    if ((dir = opendir(path)) == NULL) {
        ans[0] = 'n'; // папки не существует, клиент не изменит адрес
    } else
        ans[0] = 'y';// клиент получит одобрение на изменение адреса
    write(sock, ans, 1);
    closedir(dir);
    return 1;
}

int ls(int sock, char * path) {
    DIR* dir;
    struct dirent* de;
    char ans [256];
    bzero (ans, 256);
    if ((dir = opendir(path)) == NULL) {
        ans[0] = 'n'; // если папки не существует
    } else ans[0] = 'y';

    write (sock, ans, 1);
        // в цикле перебираем все содержимое, отправляя новое сообщения для каждого файла
    while ((de = readdir(dir)) != NULL) {
        bzero (ans, 256);
        strncat(ans, de->d_name, 256);
        if (!(strncmp(ans, "_ls_end", 256))) pastShield(ans);
        write(sock, ans, 256);
    }
    write (sock, "_ls_end", 256); // по данной строке клиент закончит прием сообщений
    closedir(dir);
    return 1;
}

int pull(int sock, char * file) {
    // *передача клиенту указанного файла от сервера*
    // проверка, есть ли этот файл
    FILE * fp;
    if((fp = fopen(file, "rb")) == NULL) {
        // если нет, то сервер отправляет отказ. клиент не начинает прием
        write(sock, "n", 1); //  отказ
        return 0;
    }
    // если да, то сервер отправляет подтверждение и начинает передачу, которую закончит спец строкой
    // подтверждение
    write(sock, "y", 1);
    // оправляем файл
    int size = 0;
    char sendbuff[256];
    bzero(sendbuff, 256);
    fseek(fp, 0, SEEK_SET);
    while (!feof(fp)) {
        size = (int)fread((void *) sendbuff, sizeof(char), 256, fp);
        if (!(strncmp(sendbuff, "_end_of_file", 256))) pastShield(sendbuff);
        write(sock, makeStrFromInt(size), 3);
        write(sock, sendbuff, size);
    }
    write(sock, makeStrFromInt(strlen("_end_of_file")), 3);
    write(sock, "_end_of_file", strlen("_end_of_file"));
    fclose(fp);
    return 1;
}

int push(int sock, char * path) {
    int size;
    // *прием сервером файла от клиента*
    // попытка открыть файл по адресу
    FILE * fp;
    if((fp = fopen(path, "wb")) == NULL) {
        // если нет, то отослать отказ. клиент не начинает передачу
        write(sock, "n", 1);
        return 0;
    }
    // если да, то отослать подтверждение, и начать прием файла. конец приема по спец строке
    write(sock, "y", 1);
    char buffer [256];
    bzero(buffer, 256);
    readn(sock, buffer, 3);
    size = atoi(buffer);
    bzero(buffer, 256);
    readn(sock, buffer, size);
    while (strncmp(buffer, "_end_of_file", 256)) {
        if (!(strncmp(buffer, "/_end_of_file", 256))) deleteShield(buffer);
        fwrite((void *) buffer, sizeof(char), size, fp);
        bzero(buffer, 256);
        readn(sock, buffer, 3);
        size = atoi(buffer);
        bzero(buffer, 256);
        readn(sock, buffer, size);
    }
    fflush(fp);
    fclose(fp);
    return 1;
}


int readn(int sockfd, char *buf, int n) {
    int k;
    int off = 0;
    for(int i = 0; i < n; ++i) {
        k = read(sockfd, buf + off, 1);
        off += 1;
        if (k < 0) {
            printf("Error reading from socket \n");
            exit(1);
        }
    }
    return off;
}

void* servConsole(void* temp) {
    int sock = *((int *) temp);
    char buffer[256];
    while(1) {
        memset(buffer, 0, 256);
        fgets(buffer, 256, stdin);
        if (!(strncmp(buffer, "close", 5))) {
            if(!(strncmp(buffer, "closeall", 8))) {
                for (int i = 0; i < 100; i++) {
                    if (flags[i] == 1) flags[i] = 2;
                }
            } else {
                char temp[3];
                temp[0] = buffer[5];
                temp[1] = buffer[6];
                temp[2] = buffer[7];
                int number = atoi(temp);
                if (flags[number] == 1) flags[number] = 2;
            }
        }
        if(!(strncmp(buffer, "exit", 4))){
            for (int i = 0; i < 100; i++) {
                if (flags[i] == 1) flags[i] = 2;
            }
            shutdown(sock, 2);
            close(sock);
            return NULL;
        }
    }
}

void* readAndWrite (void* temp) {
    int sock = *((int *) temp);
    char buf[256];
    char *p = buf;
    int flag = 0;
    write(sock, startDir, 256); //отправляем стартовую директорию клиенту
    while(1) {

        for (int i = 0; i < 100; i++) {
            if (sockets[i] == sock) {
                if (flags[i] == 2) {
                    flag = 1;
                }
                break;
            }
        }

        if (flag == 1) break;

        bzero(buf, 256);
        int n = readn(sock, p, 3);
        if (n < 0) {
            perror("ERROR on reading");
            exit(1);
        }
        int length = atoi(p);
        bzero(buf, 256);
        n = readn(sock, p, length);
        if (n < 0) {
            perror("ERROR on reading");
            exit(1);
        }
        parse(sock, buf);
    }
    shutdown(sock, 2);
    close(sock);
    return NULL;
 }


int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    uint16_t portno;
    unsigned int clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int number = 0;

    // первый вызов socket() для открытия сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    // инициализируем структуру сокета
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // вызов bind()
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    for (int i =0; i < 100; i++) {
        flags[i] = 0;
        sockets[i] = 0;
    }

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, servConsole, &sockfd);
    pthread_detach(tid);

    // ждем подсоединения клиентов
    while (1) {
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);

        // подтверждаем соединение, открываем сокет с новым дескриптором
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }
        flags[number] = 1;
        sockets[number] = newsockfd;
        number++;
        //создаем отдельный поток, инициализируем, переводим в поток общение с клиентом
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid, &attr, readAndWrite, &newsockfd);
        pthread_detach(tid);
    }

    return 0;
}