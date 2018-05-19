#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#define BUFSIZE 1024
#define EPOLL_SIZE 1000

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Start client with defalut parametres\n");
    }
    
    sockaddr_in addr;
    int sock;
    int epfd;
    static struct epoll_event ev;
    static struct epoll_event events[EPOLL_SIZE];
    int pipe_fd[2];
    char buff[BUFSIZE];
    int continue_to_work = 1;

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

    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(1);
    }

    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0) {
        perror("epoll_create");
        exit(1);
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sock;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) < 0) {
        perror("Socket connection hadn't added to epoll\n");
        exit(1);
    }
    ev.data.fd = pipe_fd[0];
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, pipe_fd[0], &ev) < 0) {
        perror("Pipe[0] (read) hadn't added to epoll\n");
        exit(1);
    }

    int len;
    if ((len = recv(sock, buff, BUFSIZE, 0)) < 0) {
        perror("recv");
        exit(1);
    }

    if (len != 0) {
	buff[len] = '\0';
        printf("%s\n", buff);        
    } else {
        continue_to_work = 0;
	printf("Client (%d) can't connect to server\n", sock);
    	if (close(sock) < 0) {
            perror("close");
            exit(1);
	}
    }
    
    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    switch (pid) {
        case 0:
            close(pipe_fd[0]);
	    printf("If you want to finish, print 'exit'\n");
            printf("Enter word and language:\n");
            while (continue_to_work) {
    		std::string query;
    		std::getline(std::cin, query);
                if (query == "exit") {
                    continue_to_work = 0;
                } else {
		    if (write(pipe_fd[1], query.data(), query.size()) < 0) {
        		perror("write");
        		exit(1);
    		    }
                }
            }
            break;
        default:
            close(pipe_fd[1]);
            while (continue_to_work) {
		int cnt = epoll_wait(epfd, events, EPOLL_SIZE, -1);
    		if (cnt < 0) {
        	    perror("epoll_wait");
        	    exit(-1);
    		}
                for (int i = 0; i < cnt; ++i) {
                    char transl[BUFSIZE];
                    if (events[i].data.fd == sock) {
                        int len;
                        if ((len = recv(sock, transl, BUFSIZE, 0)) < 0) {
                            perror("recv");
                            exit(1);
                        }
                        if (len == 0) {
                            printf("Server closed connection: %d\n", sock);
    			    if (close(sock) < 0) {
        			perror("close");
        			exit(1);
    			    }
                            continue_to_work = 0;
                        } else {
                            transl[len] = '\0';
			    printf("[DICT] %s\n", transl);
                        }
                    } else {
                        int len;
                        if ((len = read(events[i].data.fd, transl, BUFSIZE)) < 0) {
                            perror("read");
                            exit(1);
                        }
                        transl[len] = '\0';
                        if (len == 0) {
                            continue_to_work = 0;
                        } else {
                            send(sock, transl, len, 0);
                        }
                    }
                }
            }
    }

    if (pid) {
        close(pipe_fd[0]);
        close(sock);
    } else {
        close(pipe_fd[1]);
    }
    return 0;
}
