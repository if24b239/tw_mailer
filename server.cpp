#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <vector>
#include <thread>
#include <ldap.h>

#include "utils/MailerSocket.hpp"
#include "utils/Storage.hpp"
#include "utils/threadutils.h"

struct blacklist_entry {
	std::string username;
	std::string ip_address;
	int timestamp;

	bool operator==(const blacklist_entry& other) const {
		return username == other.username || ip_address == other.ip_address;
	}
};

#define VECTOR(T) thread_obj<std::vector<T>>
#define BLACKLIST_TIME 100;

struct thread_data {
    int client_sd;
	char* client_ip;
    std::string directory_name;
    VECTOR(blacklist_entry)* blacklist_ptr;
    VECTOR(Storage*)* file_ptrs;
    std::deque<Storage>* file_storage;

	std::string ip_to_string() {
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, client_ip, ip, INET_ADDRSTRLEN);
		return std::string(ip);
	}
};

Storage* get_File(std::string filename, VECTOR(Storage*)* filePointers) {
    auto ptr = filePointers->access();

    for (auto file : *ptr) {
        auto file_ptr = file->access(StorageMode::NONE);
        if (file_ptr->filename == filename) {
            return file;
        }
    }

    // if nothing is found return nullptr
    return nullptr;
}

/// @brief 
/// @param dir_path name of directory to search for files
/// @param allFiles the vector saving all thread save filenames and their fstreams.
/// @param filePointers for threads to access allFiles.
void readExistingFiles(std::string dir_path, std::deque<Storage>& allFiles, VECTOR(Storage*)* filePointers) {
    auto acc = filePointers->access();

    // fill allFiles and filePointers with the pointer to that file
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            allFiles.emplace_back(entry.path().string());
            acc->push_back(&allFiles.back());
        }
    }
}

void saveMessage(json mail, thread_data t_data) {
    std::string filename = mail["receiver"].get<std::string>() + ".txt";
    std::string path = t_data.directory_name + "/" + filename;
    
    Storage* file = get_File(path, t_data.file_ptrs);

    // if no Storage file was found create a new one and add it to fileStorage and filePointers
    if (file == nullptr) {
        auto acc = t_data.file_ptrs->access();
        t_data.file_storage->emplace_back(path);
        acc->push_back(&t_data.file_storage->back());

        // then set the newly create Storage to the current file
        file = &t_data.file_storage->back();
    }

    // Read existing data
    json data;
    {
        auto acc = file->access(StorageMode::READ);

        std::ifstream* read_stream = acc->get_ifstream();
        if (!read_stream->good()) {
            data = json::array();
        } else {
            data = json::parse(*read_stream);
        }
    }
    // Add new message
    data.push_back(mail);
    
    // Write back
    {
        auto acc = file->access(StorageMode::WRITE);

        std::ofstream* write_stream = acc->get_ofstream();

        if (!*write_stream) { std::cerr << "unable to open file: " << path << std::endl; return; }
        *write_stream << data.dump(4);
    }
    
}


std::vector<std::string> listMessages(std::string username, thread_data data) {
    std::string path = data.directory_name + "/" + username + ".txt"; //specifiy correct path to file
    std::vector<std::string> subjects;

    Storage* file = get_File(path, data.file_ptrs);
    if (file == nullptr) {
        
        std::cerr << "unknown file: " << path << std::endl;
        subjects.push_back("no messages");
        return subjects;
    }

    {
        auto acc = file->access(StorageMode::READ);

        std::ifstream* read_stream = acc->get_ifstream();
        if (!read_stream->good()) {
            std::cerr << "couldn't open file: " << path << std::endl;
            subjects.push_back("file error");
            return subjects;
        }

        // get the json data and list all subjects if file open
        json data = json::parse(*read_stream);
        if (data.size() == 0) {
            subjects.push_back("no messages");
            return subjects;
        }

        for (const auto& message : data) {
            subjects.push_back(message["subject"].get<std::string>());
        }
    }
    
    return subjects;
}

Mail returnMessage(std::string username, int number, thread_data t_data) {
    std::string path = t_data.directory_name + "/" + username + ".txt"; //specifiy correct path to file
    json data;
    
    Storage* file = get_File(path, t_data.file_ptrs);
    if (file == nullptr) {
        Mail mail("no user found","no user found","no user found","no user found");
        return mail;
    }

    {
        auto acc = file->access(StorageMode::READ);

        std::ifstream* read_stream = acc->get_ifstream();
        if (!read_stream->good()) {
            Mail mail("couldn't open file","couldn't open file","couldn't open file","couldn't open file");
            return mail;
        }

        data = json::parse(*read_stream);
        if (data.size() == 0) {
            Mail mail("no messages to read","no messages to read","no messages to read","no messages to read");
            return mail;
        }
    }
    
    return data.at(number-1).get<Mail>();
}

bool deleteMessage(std::string username, int number, thread_data t_data) {
    std::string path = t_data.directory_name + "/" + username + ".txt"; //specifiy correct path to file
    json data;

    Storage* file = get_File(path, t_data.file_ptrs);
    if (file == nullptr) {
        std::cout << "unknown user: " << path << std::endl; 
        return false;
    }

    {
        auto acc = file->access(StorageMode::READ);

        std::ifstream* read_stream = acc->get_ifstream();
        if (!read_stream->good()) {
            std::cout << "unable to open file: " << path << std::endl; 
            return false;
        }

        
        data = json::parse(*read_stream);
        if (data.size() == 0) {
            std::cout << "No Messages to be deleted in file: " << path << std::endl;
            return false;
        }
    }

    data.erase(data.begin() + number-1);

    {
        auto acc = file->access(StorageMode::WRITE);

        std::ofstream* write_stream = acc->get_ofstream();
        if (!write_stream->good()) {
            std::cout << "unable to open file: " << path << std::endl;
            return false;
        }

        *write_stream << data.dump(4);
    }

    return true;
}

bool login(std::string username, std::string password) {

    LDAP *ldapHandle;
    const char *ldapUri = "ldap://ldap.technikum-wien.at:389";
    const int ldapVersion = LDAP_VERSION3;

    // start ldap connection
    int rc = ldap_initialize(&ldapHandle, ldapUri);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "LDAP init failed: " << ldap_err2string(rc) << "\n";
        return false;
    }
    if (ldapHandle == nullptr) {
        std::cerr << "LDAP handle is NULL\n";
        return false;
    }

    rc = ldap_set_option(
        ldapHandle,
        LDAP_OPT_PROTOCOL_VERSION, // OPTION
        &ldapVersion);             // IN-Value
    if (rc != LDAP_OPT_SUCCESS) {
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(rc));
        ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);
        return false;
    }

    // start TLS
    rc = ldap_start_tls_s(ldapHandle, NULL, NULL);
    if (rc != LDAP_SUCCESS) {
        std::cerr << "TLS start failed: " << ldap_err2string(rc) << "\n";
        return false;
    }

    // get DN form username
    std::string dn = "uid=" + username + ",ou=people,dc=technikum-wien,dc=at";

    // connect with ldap server
    // credentials to send
    BerValue clientCreds;
    clientCreds.bv_val = (char*)password.c_str();
    clientCreds.bv_len = strlen(password.c_str());
    
    rc = ldap_sasl_bind_s(
        ldapHandle,
        dn.c_str(),
        LDAP_SASL_SIMPLE,
        &clientCreds,
        nullptr,
        nullptr,
        nullptr
    );
    if (rc != LDAP_SUCCESS) {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(rc));
        return false;
    }

    ldap_unbind_ext_s(ldapHandle, nullptr, nullptr);

    return true;
}

bool is_blacklisted(std::string username, std::string ip, VECTOR(blacklist_entry)* blacklist) {
    auto bl_acc = blacklist->access();

	blacklist_entry compare_entry;
	compare_entry.username = username;
	compare_entry.ip_address = ip;

    for (auto e : *bl_acc) {
        if (e == compare_entry) { 
            return true;
        }
    }
    return false;
}

void threadedConnection(thread_data data) {
    // handle client connection in a separate thread
    
    std::string username = "";
    unsigned int failedAttempts = 0;

    char buffer[1024] = {0};
    for (;;) {
        std::string recvMsg;
        ssize_t recvd;
        
        while(true){    //loop to ensure large messages can be received
            recvd = recv(data.client_sd, buffer, sizeof(buffer) - 1, 0);

            if (recvd == -1) {
            perror("recv error");
            continue; // wait for a new datapacket
            }
            if (recvd == 0) { // connection closed by client
                std::cout << "Client disconnected.\n";
                break;
            }

            buffer[recvd] = '\0';
            recvMsg += buffer;

            try {
                json message = json::parse(recvMsg);
                break; //exit loop if parsing success
            } catch (const json::parse_error&) {    //stay in loop in case of incomplete JSON
            }
        }
        if(recvd == 0) break;
        json message = json::parse(recvMsg.c_str());
        json returnMsg = json::object();

        std::string c;
        ReceiveType l = TYPE_NONE;

        switch (message["receive_type"].get<int>())
        {
        case SEND:
            saveMessage(message["mail"], data);
            c = "OK\n";
            break;
        case LIST:
            {
                int count = 0;
                std::string subjects = "";
                //std::vector<std::string> msgSubjects =listMessages(message["content"].get<std::string>(), data.directory_name);
                for (auto s : listMessages(message["content"].get<std::string>(), data)) {
                    //if(count == 0 && s == "not found"){ //chk for placeholder identifying no mails found
                    if(count == 0 && s == "no messages"){ //chk for placeholder identifying no mails found
                        c = "ERR\n";
                        continue;
                    }
                    subjects += s;
                    subjects += "\n";
                    count++;
                }
                ;
                if(c != "ERR\n"){   //if no mails found no need to count
                    c += "Count of Messages: " + std::to_string(count) + '\n';
                    c += subjects;
                }
            }
            break;
        case READ:
            {
                //chk whether mail was found
                Mail chkMail = returnMessage(message["content"].get<std::string>(), message["number"].get<int>(), data);
                c = (chkMail.getSender() == "no messages to read") ? "ERR\n" : returnMessage(message["content"].get<std::string>(), message["number"].get<int>(), data).serialize();
            }
            break;
        case DEL:
            //chk whether deletion successful
            c = (deleteMessage(message["content"].get<std::string>(), message["number"].get<int>(), data) == true) ? "OK\n" : "ERR\n";
            break;
        case LOGIN:
            // end switch when the client is blacklisted
            if (is_blacklisted(message["content"]["username"], data.ip_to_string(), data.blacklist_ptr)) {
                c = "ERR blacklisted\n";
                break;
            }

            // check if login data is correct
            if (login(message["content"]["username"], message["content"]["password"])) {
                c = "OK\n";
                username = message["content"]["username"];
                failedAttempts = 0;
            } else {
                // handle failed access attempts and blacklisting
                failedAttempts++;
                c = "ERR " + std::to_string(failedAttempts) + '\n';
                if (failedAttempts >= 3) {
                    auto bl_acc = data.blacklist_ptr->access();
                    blacklist_entry new_entry;
                    new_entry.username = message["content"]["username"];
                    new_entry.ip_address = data.ip_to_string();
                    new_entry.timestamp = static_cast<int>(time(nullptr));
                    bl_acc->push_back(new_entry);

                    failedAttempts = 0;
                }
            };
            l = LOGIN;
            break;
        
        default:
            perror("INVALID RETURN JSON");
            c = "ERR\n";
            break;
        }
        std::memset(buffer, 0, sizeof(buffer)); //clear buffer after each message

        nlohmann::json j;
        j["receive_type"] = ReceiveType::REPLY | l;
        j["content"] = c;
        std::string msg = j.dump();
        const char* messag = msg.c_str();
        if (send(data.client_sd, messag, strlen(messag), 0) == -1) {
            perror("send error");
            exit(1);
        }
    }
        
    close(data.client_sd);
}

void blacklist_thread(VECTOR(blacklist_entry)* blacklist) {
	while (true) {
		std::this_thread::sleep_for(std::chrono::minutes(1));
		// get current time
		int current_time = static_cast<int>(time(nullptr));

		auto bl_acc = blacklist->access();
		bl_acc->erase(
			std::remove_if(
				bl_acc->begin(),
				bl_acc->end(),
				[current_time](const blacklist_entry& entry) {
					return (current_time - entry.timestamp) > BLACKLIST_TIME; // remove if entry is older than BLACKLIST_TIME seconds
				}
			),
			bl_acc->end()
		);
	}	
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <directory_name>" << "\n";
        return 1;
    }

    std::string directory_name = argv[2];
    MailerSocket serverSocket = MailerSocket(std::stoi(argv[1]));
    std::vector<std::thread> allThreads;
    VECTOR(blacklist_entry) blacklist;
    VECTOR(Storage*) filePointers;

    //!!!! ONLY ACCESS WHILE filePointers IS LOCKED TO ENSURE NO RACECONDITIONS!!!!
    std::deque<Storage> fileStorage;

    // get accessors for all files
    readExistingFiles(directory_name, fileStorage, &filePointers);

    // start binding socket/port
    if (bind(serverSocket.getDescriptor(), serverSocket.getSockAddr(), serverSocket.getSockAddrLen()) == -1) {
        perror("bind error");
        exit(1);
    }

    if (listen(serverSocket.getDescriptor(), SOMAXCONN) == -1) {
        perror("listen error");
        exit(1);
    }

	// starting blacklist cleanup thread
	allThreads.push_back(std::thread(blacklist_thread, &blacklist));

    /* Binding and listening info */
    system("echo -n 'Local IP addr: ';ip addr show | grep 'eth0' | grep 'inet' | awk '{print $2}' | cut -d'/' -f1");

    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(serverSocket.getDescriptor(), (sockaddr*)&addr, &len) == 0) {
        char s[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, s, sizeof(s));
        std::cout << "Bound to " << s << "\n";
    }
    std::cout << "Server listening on port " << serverSocket.getPort() << "\n";
    
    sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    for (;;) {
        // accept
        int client_sd = accept(serverSocket.getDescriptor(), (sockaddr*) &client_addr, &addrlen);
        if (client_sd == -1) {
            perror("accept error");
            continue; // try accepting again
        }
        std::cout << "Client connected.\n";

        // make thread handeling new connection
        thread_data data = {};
        data.client_sd = client_sd;
		data.client_ip = inet_ntoa(client_addr.sin_addr);
        data.directory_name = directory_name;
        data.blacklist_ptr = &blacklist;
        data.file_ptrs = &filePointers;
        data.file_storage = &fileStorage;

        allThreads.push_back(std::thread(threadedConnection, data));
    }


    /*
    CLEANUP
    */
    for (auto& t : allThreads) {
        if (t.joinable())
            t.join();
    }

    return 0;
}