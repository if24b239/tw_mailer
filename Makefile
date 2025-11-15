TARGET=./bin/twmailer
FLAGS=-std=c++23 -I./utils -g -Wall
CLIENT=$(TARGET)-client
SERVER=$(TARGET)-server
SOCKET=$(TARGET)-socket.o
PARSER=$(TARGET)-parser.o

all: $(CLIENT) $(SERVER) modules

$(CLIENT): $(CLIENT).o $(SOCKET) $(PARSER)
	g++ $(FLAGS) -o $(CLIENT) $(CLIENT).o $(SOCKET) $(PARSER)

$(SERVER): $(SERVER).o $(SOCKET) $(PARSER)
	g++ $(FLAGS) -o $(SERVER) $(SERVER).o $(SOCKET) $(PARSER)


# Compile .o Files
$(CLIENT).o: client.cpp
	g++ $(FLAGS) -c client.cpp -o $(CLIENT).o

$(SERVER).o: server.cpp
	g++ $(FLAGS) -c server.cpp -o $(SERVER).o

$(SOCKET): utils/MailerSocket.cpp utils/MailerSocket.hpp
	g++ $(FLAGS) -c utils/MailerSocket.cpp -o $(SOCKET)

$(PARSER): utils/Parser.cpp utils/Parser.hpp
	g++ $(FLAGS) -c utils/Parser.cpp -o $(PARSER)

modules: .gitmodules
	git submodule update --init --recursive

# Cleanup
clean: clean_o
	rm -f $(CLIENT) $(SERVER)

clean_o :
	rm -f ./bin/*.o