# Welcome to My Ftp
***

## Task
The task is to create an FTP server that follows the **File Transfer Protocol (FTP)** following the protocol **RFC 959**. 
The challenge lies in implementing the server to handle multiple clients simultaneously using **TCP sockets** and to support both **active** and **passive** data transfer modes as in the FTP protocol.

## Description
The FTP server has the following features:

- **User Authentication**: Users must log in with the `USER` and `PASS` commands before they can interact with the server.
- **Data Transfer Modes**: The server supports two data transfer modes: **Active** and **Passive**.
    - **Active Mode (PORT)**: The client provides the server with a port and IP address to connect to for data transfer.
    - **Passive Mode (PASV)**: The server provides the client with an IP address and port to connect to for data transfer.
- **File Commands**: The `RETR` (retrieve) and `STOR` (store) commands can only be used after establishing a data connection in either Active or Passive mode.
- **Thread Pool for Managing Multiple Clients**: The server uses a **Thread Pool** to manage and handle multiple client connections simultaneously. 
- **Other Commands**: The server supports a variety of commands, including:
    - **PORT**: Specifies the IP address and port the client will use for an incoming data connection (Active mode).
    - **PASV**: The server enters Passive mode and provides the client with an IP address and port to connect to for data transfer.
    - **LIST**: Lists the contents of the current directory.
    - **QUIT**: Ends the session with the server.
    - **CD**: Changes the current directory on the server.
    - **PWD**: Displays the current directory on the server.
    - **RETR**: Retrieves (downloads) a file from the server.
    - **STOR**: Stores (uploads) a file to the server.
    - **USER**: Authenticates the user (starts the login process).
    - **PASS**: Provides the password for user authentication.

## Installation
1. Compile the server with:
```
make
```
2. Compile the test client with:
```
g++ -o client src/client.cpp
```
## Usage
THE SERVER:
1. You can run the server with "./server" followed by the [PORT NUMBER] that the server will run on and the [PATH] of the directory the server shall run on.
```
./server 8080 .

will run the server on port 2121 and with the path of the current directory.
```

THE CLIENT:
1. Run the client with ./client followed by the [PORT NUMBER]
```
./client 8080

you will then be connected to the server specified in the example code above
```

3. You will then connect to the server and you need to log in with an username and a password, the only ones allowed are 'Anonymous' and empty PASS
```
Connected to server on 127.0.0.1:8080
220 Service ready for new user. Please Log In
ftp> USER Anoymous
331 User name okay, need password.
ftp> PASS 
230 User logged in, proceed.
ftp>
```

4. After logged in you can use the following commands:
```
* "PWD" - displays the filepath of the current directory the client is accessing.

* "CD [PATH]" - changes directory to the specified directory, ".." will move to the parent directory if
possible.

* "PASV" - changes the connection to passive mode and will display the IP and port location of the 
server before executing the next command.

* "PORT [IP][PORT]" - changes the connection to active mode, specifies to the server the IP address the client is running on and port the client will use for an incoming data connection. Ex: PORT 127,0,0,1,54,23

* "LIST" - will list the contents of the current directory.

* "LIST [PATH]" - will list the contents of the specified path in [PATH].  Ex: LIST test

* "RETR [FILENAME]" - will retrieve the specified file and save it to the current directory of the client. Ex: RETR test/test.txt

* "STOR [FILENAME]" - will send the specified file and save it to the current directory of the server.
   Ex: if you run the server on the dir test: ./server 8080 test 
       and then you send a file that is in the . dir the file will send to the test dir: STOR test_stor_client.txt

* "QUIT" - will close the client
```
### The Core Team


<span><i>Made at <a href='https://qwasar.io'>Qwasar SV -- Software Engineering School</a></i></span>
<span><img alt='Qwasar SV -- Software Engineering School's Logo' src='https://storage.googleapis.com/qwasar-public/qwasar-logo_50x50.png' width='20px' /></span>