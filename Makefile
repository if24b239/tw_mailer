TARGET=./bin/twmailer
FLAGS=-std=c++23 -I./utils -g -Wall
CLIENT=$(TARGET)-client
SERVER=$(TARGET)-server
SOCKET=$(TARGET)-socket.o

all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT).o $(SOCKET)
	g++ $(FLAGS) -o $(CLIENT) $(CLIENT).o $(SOCKET)

$(SERVER): $(SERVER).o $(SOCKET)
	g++ $(FLAGS) -o $(SERVER) $(SERVER).o $(SOCKET)


# Compile .o Files
$(CLIENT).o: client.cpp
	g++ $(FLAGS) -c client.cpp -o $(CLIENT).o

$(SERVER).o: server.cpp
	g++ $(FLAGS) -c server.cpp -o $(SERVER).o

$(SOCKET): utils/MailerSocket.cpp utils/MailerSocket.hpp
	g++ $(FLAGS) -c utils/MailerSocket.cpp -o $(SOCKET)

# Cleanup
clean: clean_o
	rm -f $(CLIENT) $(SERVER)

clean_o :
	rm -f ./bin/*.o