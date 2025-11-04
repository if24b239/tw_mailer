TARGET=./bin/twmailer
FLAGS=-std=c++23
CLIENT=$(TARGET)-client
SERVER=$(TARGET)-server

all: $(CLIENT) $(SERVER)

$(CLIENT): client.cpp
	g++ $(FLAGS) -o $(CLIENT) client.cpp

$(SERVER): server.cpp
	g++ $(FLAGS) -o $(SERVER) server.cpp

clean:
	rm -f ./bin/*