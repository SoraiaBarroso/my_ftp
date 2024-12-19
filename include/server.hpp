#ifndef SERVER_HPP 
#define SERVER_HPP 
    #include <fcntl.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <cstddef>
    #include <cstdlib>
    #include <cstring>
    #include <filesystem>  
    #include <iostream>
    #include <poll.h>
    #include <vector>

    #include "command_parser.hpp"
    #include "command_handler.hpp"
    #include "thread_pool.hpp"
    #include "ftp_messages.hpp"
    	
    #define MAX_PENDING_CONNECTIONS 5

    // Manage server
    class Server {	
        public:	
            Server(int port, std::string path) : portNbr(port), homeDir(path), threadPool() { };	

            // Initialize the server with the given port and path
            void startServer() {
                setOriginPath();	
                initSocket();	
                acceptConnections();	
            };	

            void stopServer() {	
                LOG_INFO("Shutting down the server...");	
                threadPool.stop();	
                close(serverSocket);	
                LOG_INFO("Server stopped");	
            }	

        private:	

            void initSocket() {	
                // Create and asign server socket
                serverSocket = socket(AF_INET, SOCK_STREAM, 0);	
                if (serverSocket < 0) {	
                    perror("Error creating socket");	
                    exit(EXIT_FAILURE);	
                }	

                serverAddress.sin_family = AF_INET;	
                serverAddress.sin_port = htons(portNbr);	
                serverAddress.sin_addr.s_addr = INADDR_ANY;	

                LOG_INFO(("Socket created successfully on port " + std::to_string(portNbr)).c_str());	

                // Bing the server socket
                if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {	
                    std::cerr << "Binding failed\n";	
                    close(serverSocket);	
                    exit(EXIT_FAILURE);	
                }	

                // Listen for connections
                if (listen(serverSocket, MAX_PENDING_CONNECTIONS) < 0) {	
                    std::cerr << "Listening failed\n";	
                    close(serverSocket);	
                    exit(EXIT_FAILURE);	
                }	

                LOG_INFO(("Server listening on port " + std::to_string(portNbr)).c_str());	
            };	

           	
            void acceptConnections() {	
                while (true) {	
                    // If server socket has activity, it's a new connection
                    struct sockaddr_in clientAddr;	
                    socklen_t addrlen = sizeof(clientAddr);	
                    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrlen);	
                        
                    if (clientSocket < 0) {	
                        std::cerr << "Error accepting client connection\n";	
                        continue;	
                    }	

                    LOG_INFO(("Client connected on socket " + std::to_string(clientSocket)).c_str());	

                    // Enqueue the task in the thread pool to handle this client
                    threadPool.enqueue([this, clientSocket] { handleClient(clientSocket); });	
                
                }	

                close(serverSocket); 	
            }	

            void handleClient(int clientSocket) {	
                message.sendMessage(clientSocket, FTPMessages::serviceReady());	

                // Step 1: authentificate User 
                if (!authentificateClient(clientSocket)) {	
                    std::cerr << "Authentification failed" << std::endl;	
                    close(clientSocket);
                    exit(1);	
                }	

                LOG_INFO("Client logged in correctly");	
                	
                // Step 2: receive commands from User
                handleCommands(clientSocket);	
            };	
            	
            // Handle user commands
            void handleCommands(int clientSocket);	
            	
            // Handle user authentification
            bool authentificateClient(int clientSocket);	

            // Put the server in the path specified
            void setOriginPath() {
                char path[100]; 

                if (getcwd(path, sizeof(path)) == nullptr) {
                    perror("Error getting current directory");
                    return;
                }

                originPath = path;

                if (homeDir == ".") {
                    homeDir = originPath;
                    LOG_INFO("Server's working directory set to: " + homeDir);
                }
                else {
                    std::string newPath = std::string(path) + "/" + homeDir;

                    if (chdir(newPath.c_str()) != 0) {
                        perror("Error changing directory");
                        return;
                    }

                    LOG_INFO("Server's working directory set to: " + newPath);
                }
            }
     
           	// Socket for passive mode
            int passiveSocket = -1;  
            sockaddr_in passiveAddress{};	

            // Socket for active mode
            int activeSocket = -1;
            sockaddr_in activeAddress{}; 

            sockaddr_in serverAddress;	
            int portNbr;	
            int serverSocket;	
            std::string homeDir;	
            std::string originPath;
            	
            CommandParser parser;	
            eCommandType commandType;	
            ThreadPool threadPool;	
            CommandHandler commandHandler;	
            FTPMessages message;	
    };	
#endif