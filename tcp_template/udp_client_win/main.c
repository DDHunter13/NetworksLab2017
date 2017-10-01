#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <winsock2.h>
#include <stdint.h>

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
    //char *p = buffer;

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(0);
    }

    portno = (uint16_t) atoi(argv[2]);

    /* Create a socket point */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

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

    /* Now ask for a message from the user, this message
       * will be read by server
    */

    printf("Please enter the message: ");
    memset(buffer, 0, 256);
    fgets(buffer, 255, stdin);

    /* Send message to the server */
    n = sendto(sockfd, buffer, 256, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    } 

    /* Now read server response */
    memset(buffer, 0, 256);
    //n = read(sockfd, buffer, 255);
    //
    //if (n < 0) {
    //    perror("ERROR reading from socket");
    //    exit(1);
    //}
    struct sockaddr_in serv2;
    int serv2_size = sizeof(serv2);    


    n = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *) &serv2, &serv2_size);
    if (n < 0) {
        perror("ERROR reading from socket");
        exit(1);
    }
    printf("%s\n", buffer);

    closesocket(sockfd);
    WSACleanup();
    return 0;
}
