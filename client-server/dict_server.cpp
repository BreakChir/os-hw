#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <map>

#define BUFSIZE 100

std::map<std::string, std::string> dict[2];
char flag = 0; // 0 - into rus, 1 - into eng, 2 - error
std::string err_msg;

void fillMap() {
    std::ifstream file("rus-eng");
    std::string s1, s2;
    while (file >> s1 >> s2) {
	dict[1][s1] = s2;
	dict[0][s2] = s1;
    }
    file.close();
}

std::string parseQuery(char buf[], size_t sz) {
    for (size_t i = 0; i < sz; ++i) {
	if (buf[i] == ' ') {
	    buf[i] = '\0';
	    std::string word = buf;
	    if (++i < sz) {
	        if (buf[i] == 'r') {
		    flag = 0;
		} else if (buf[i] == 'e') {
		    flag = 1;
		} else {
		    flag = 2;
		    err_msg = "Unacceptable language";
		}
	    } else {
	        break;
	    }
	    return word;
	}
    }
    flag = 2;
    err_msg = "Incorrect input. Start with argument: {--help}";
}



void execute(int sock) {
    char buf[BUFSIZE];
    size_t sz;
    sz = (size_t) recv(sock, buf, BUFSIZE, 0);
    if (sz <= 0) {
	return;
    }
          
    std::string word = parseQuery(buf, sz);
    std::string translate;

    if (flag != 2) {
        if (dict[flag].count(word)) {
            translate = dict[flag][word];
        } else {
            flag = 2;
            err_msg = "Not found input word in dictionary";
	}
    }
	
    if (flag == 2) {
        send(sock, err_msg.data(), err_msg.length(), 0);
    } else {
        send(sock, translate.data(), translate.length(), 0);
    }
}


int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        printf("The client starts with two arguments:\n");
        printf("The first argument is number of Port\n");
        printf("The second argument is Internet address\n\n");
	printf("If you won't write arguments server will start with default properties:\n");
	printf("The port will be 3425\n");
	printf("The Internet address is const: INADDR_ANY\n");
	exit(1);
    }
    if (argc != 3) {
        printf("Start server with one argument {--help} to read info\n");
    }
    int listener;
    sockaddr_in addr{};

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
    }

    addr.sin_family = AF_INET;
    if (argc != 3) {
        addr.sin_port = htons(3425);
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_addr.s_addr = inet_addr(argv[2]);
    }

    if (bind(listener, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);

    fillMap();

    printf("Server is started\n");
    for (int i = 0; i < 5; ++i) {
        int sock = accept(listener, nullptr, nullptr);
        if (sock < 0) {
            perror("accept");
            exit(3);
        }
        execute(sock);
	close(sock);
    }

    printf("Limit of queries is reached : 5\n");
    close(listener);
    return 0;
}
