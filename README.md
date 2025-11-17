*Overview*

Our tw_mailer project is a distributed email system implemented in C++23 that enables clients to send, retrieve, list, and delete messages through a TCP/IP socket-based server. The system uses JSON for protocol communication and persistent file storage.

*Architecture*

Client-Server Model
The project follows a request-response architecture:

**Server (server.cpp)**:

- Listens on a configurable port (default 6000) bound to 0.0.0.0 (all interfaces)
- Accepts TCP connections sequentially (single-threaded)
- Processes incoming JSON requests and executes file operations
- Returns JSON-encoded responses to clients
- Stores user mailboxes in a directory as JSON-formatted files

**Client (client.cpp)**:

- Connects to the server via IP address and port
- Sends user commands (SEND, LIST, READ, DEL, QUIT) as JSON payloads
- Receives and displays server responses
- Terminates on QUIT command or connection loss

**Data Flow**

Client Request (JSON)
    ↓
Network (TCP/IP)
    ↓
Server Parser & Router
    ↓
File Operations (read/write JSON)
    ↓
Response (JSON)
    ↓
Client Display

**Technologies used**

Language: C++23
Networking: TCP/IP
Serialization of Data: nlohmann/json (https://github.com/nlohmann/json)
Data Storage: File System

*How to*

Should you get errors like "git@github.com: Permission denied (publickey). Run the following command in this Directory
git config url."https://github.com/".insteadOf "git@github.com:"