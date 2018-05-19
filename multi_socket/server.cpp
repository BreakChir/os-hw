#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

#define BUFSIZE 1024
#define EPOLL_SIZE 1000

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

std::string parseQuery(char buf[], int sz) {
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
    err_msg = "Incorrect input. Read info";
}

void handle_message(int sock) {
    char buf[BUFSIZE];
    int sz;
    sz = recv(sock, buf, BUFSIZE, 0);
    if (sz < 0) {
        perror("recv");
        exit(1);
    }

    if (sz == 0) {
	printf("Client with fd: %d closed!\n", sock);
    	if (close(sock) < 0) {
            perror("close");
            exit(1);
    	}
    } else {
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
}

void set_non_blocking(int listener) {
    if (fcntl(listener, F_SETFL, fcntl(listener, F_GETFD, 0) | O_NONBLOCK) < 0) {
        perror("fcntl");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int listener;
    sockaddr_in addr{};
    int epfd;
    static struct epoll_event ev;
    static struct epoll_event events[EPOLL_SIZE];

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    set_non_blocking(listener);

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
        exit(1);
    }

    if (listen(listener, 1) < 0) {
        perror("listen");
        exit(1);
    }

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epoll_create");
        exit(1);
    }

    ev.data.fd = listener;
    ev.events = EPOLLIN | EPOLLET;
    
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
    }

    fillMap();	

    if (argc != 3) {
        printf("Server is started with default parametres\n");
    } else {
	printf("Server is started with port = %d, Internet address = %s\n", atoi(argv[1]), argv[2]);	
    }

    for (int i = 0; i < 100; ++i) {
        int cnt = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if (cnt < 0) {
            perror("epoll_wait");
            exit(1);
        }

	for (int i = 0; i < cnt; ++i) {
            if (events[i].data.fd == listener) {
		int client = accept(listener, nullptr, nullptr);
    		if (client < 0) {
        	    perror("accept");
        	    exit(1);
    		}

                printf("Add new client(fd = %d)\n", client);
                set_non_blocking(client);

                ev.data.fd = client;
		if (epoll_ctl(epfd, EPOLL_CTL_ADD, client, &ev) < 0) {
       		    perror("epoll_ctl");
        	    exit(1);
    		}

                std::string message = "DICT Protocol\n";
    		send(client, message.data(), message.size(), 0);
            } else {
                handle_message(events[i].data.fd);
            }
        }
    }

    close(listener);
    close(epfd);
    return 0;
}
