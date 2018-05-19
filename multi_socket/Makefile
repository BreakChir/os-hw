.PHONY: server client clean

server: serv
	./serv

serv: server.cpp
	g++ -std=c++14 $^ -o $@

client: cl
	./cl

cl: client.cpp
	g++ -std=c++14 $^ -o $@

clean:
	rm -rf serv cl
