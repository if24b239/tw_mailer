TARGET=./bin/twmailer
FLAGS=-std=c++23 -I./utils -I./json/single_include/nlohmann -g -Wall
LINKER=-lldap -llber
CLIENT=$(TARGET)-client
SERVER=$(TARGET)-server
SOCKET=$(TARGET)-socket.o
PARSER=$(TARGET)-parser.o
MAIL=$(TARGET)-mail.o

all: modules $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT).o $(SOCKET) $(MAIL)
	g++ $(FLAGS) -o $(CLIENT) $(CLIENT).o $(SOCKET) $(MAIL)

$(SERVER): $(SERVER).o $(SOCKET) $(MAIL)
	g++ $(FLAGS) -o $(SERVER) $(SERVER).o $(SOCKET) $(MAIL) $(LINKER)

# Compile .o Files
$(CLIENT).o: client.cpp
	g++ $(FLAGS) -c client.cpp -o $(CLIENT).o

$(SERVER).o: server.cpp
	g++ $(FLAGS) -c server.cpp -o $(SERVER).o

$(SOCKET): utils/MailerSocket.cpp
	g++ $(FLAGS) -c utils/MailerSocket.cpp -o $(SOCKET)

$(PARSER): utils/Parser.cpp
	g++ $(FLAGS) -c utils/Parser.cpp -o $(PARSER)

$(MAIL): utils/Mail.cpp
	g++ $(FLAGS) -c utils/Mail.cpp -o $(MAIL)

modules: .gitmodules
	git submodule update --init --recursive

# Cleanup
clean: clean_o
	rm -f $(CLIENT) $(SERVER)

clean_o :
	rm -f ./bin/*.o