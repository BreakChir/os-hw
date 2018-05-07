#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <iostream>

#define BUFSIZE 100

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("The client starts with two arguments:\n");
        printf("The first argument is number of Port\n");
        printf("The second argument is Internet address\n\n");
	printf("If you won't write arguments client will start with default properties:\n");
	printf("The port will be 3425\n");
	printf("The Internet address is const: INADDR_LOOPBACK\n\n");
	printf("Client's query has this pattern:\n");
	printf("[word] [language]\n");
	printf("[word] - word, which need to be translated into [language]: {rus} {eng}\n");
	printf("Its' supported only russian and english\n");
	exit(1);
    }
    if (argc != 3) {
        printf("Start client with one argument {--help} to read info\n");
    }

    int sock;
    sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
    }
    
    addr.sin_family = AF_INET;
    if (argc != 3) {
        addr.sin_port = htons(3425);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    } else {
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr(argv[2]);    
    }

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(2);
    }

    std::string query;
    printf("Enter word and language:\n");
    std::getline(std::cin, query);

    char buf[BUFSIZE];
    send(sock, query.data(), query.length(), 0);
    size_t sz = recv(sock, buf, BUFSIZE, 0);
    buf[sz] = '\0';

    printf("%s\n", buf);
    
    close(sock);

    return 0;
}
