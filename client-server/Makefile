.PHONY: server client clean

server: serv
	./serv

serv: dict_server.cpp
	g++ -std=c++14 $^ -o $@

client: cl
	./cl

cl: dict_client.cpp
	g++ -std=c++14 $^ -o $@

clean:
	rm -rf cl serv
