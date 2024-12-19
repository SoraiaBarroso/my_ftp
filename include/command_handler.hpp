#ifndef COMMAND_HANDLER_HPP
#define COMMAND_HANDLER_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/socket.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include "ftp_messages.hpp"
#include "thread_pool.hpp"

class CommandHandler {

    public:
        CommandHandler() = default;

        std::string pwd(int clientSocket) {
            char currDir[200];
            
            if (getcwd(currDir, sizeof(currDir)) == nullptr) {
                message.sendMessage(clientSocket, "550 PWD command failed.\r\n");
                return nullptr;
            }
            
            std::string pwdResponse = std::string(currDir) + "\n";
            message.sendMessage(clientSocket, pwdResponse);

            return pwdResponse;
        }

        void ls(int clientSocket, std::vector<std::string> commands) {
            std::string command = "ls -l";

            if (commands.size() == 2) {
                command += " " + commands[1];
            } 

            FILE *fp = popen(command.c_str(), "r");

            if (fp == NULL) {
                message.sendMessage(clientSocket, "Error: Unable to execute ls command.\r\n");
                return;
            }

            char buffer[256];
            std::string result;
            while (fgets(buffer, sizeof(buffer), fp) != NULL) {
                result += buffer; 
            }

            result + "\n";

            fclose(fp);

            message.sendMessage(clientSocket, result);
        }

        // Restricted Root Director: client cannot navigate outside home/docode/project
        void cd(int clientSocket, std::vector<std::string> commands, std::string originPath) {
            std::string targetPath;

            // if user inputs cd ~ or only cd set the targetPath as the origin of the server
            if (targetPath == "~" || commands.size() == 1) {
                targetPath = originPath;
            } else {
                targetPath = commands[1];
            }

            char resolvedPath[200];
            if(realpath(targetPath.c_str(), resolvedPath) != NULL) {
                if(std::string(resolvedPath).find(originPath) == 0) {
                    if (chdir(resolvedPath) != 0) {
                        message.sendMessage(clientSocket, "Error: unable to change directory\r\n");
                    }
                } else {
                    message.sendMessage(clientSocket, "Error: Access denied\r\n");
                }
            } else {
                message.sendMessage(clientSocket, "Error: Directory doesn't exist\r\n");
            }
            
            
            message.sendMessage(clientSocket, "Directory changed\r\n");
        };

        void quit(int clientSocket) {
            message.sendMessage(clientSocket, "Exiting the server...\r\n");

            close(clientSocket);
            LOG_INFO("Client disconnected");
            exit(0); 
        }

        //  This command requests the server to open a random port for data transfer, listens for a single connection and then closes it afterward
        void passiveMode(int clientSocket, int& passiveSocket, sockaddr_in& passiveAddress) {
            passiveSocket = socket(AF_INET, SOCK_STREAM, 0);

            if (passiveSocket < 0) {
                perror("Error creating passive socket");
                exit(1);
            }
            
            passiveAddress.sin_family = AF_INET;
            passiveAddress.sin_port = 0;
            
            // Convert IP address string to binary form
            if (inet_pton(AF_INET, "127.0.0.1", &passiveAddress.sin_addr) <= 0) {
                perror("Invalid IP address");
                return;
            }

            if (bind(passiveSocket, (struct sockaddr*)&passiveAddress, sizeof(passiveAddress)) < 0) {
                perror("Error binding passive socket");
                close(passiveSocket);
                passiveSocket = -1;
                return;
            }

            // Get the port asigned by the system
            socklen_t addrLen = sizeof(passiveAddress);
            if (getsockname(passiveSocket, (struct sockaddr*)&passiveAddress, &addrLen) < 0) {
                perror("Error getting passive socket info");
                close(passiveSocket);
                passiveSocket = -1;
                return;
            }
            
            // Start listening to the passive socket
            if (listen(passiveSocket, 5) < 0) {
                perror("Error listening on passive socket");
                close(passiveSocket);
                passiveSocket = -1;
                return;
            }

            LOG_INFO("Passive socket is listening.");

            // Send response to the client 
            int port = ntohs(passiveAddress.sin_port);
            int ip = ntohl(passiveAddress.sin_addr.s_addr);

            LOG_INFO("IP: " + std::to_string(ip));
            LOG_INFO("PORT: " + std::to_string(port));

            // IP address (32-bit) in dotted-decimal format (a1,a2,a3,a4,p1,p2) and port nbr in 16-bit split into 8-bit numbers
            std::string response = "227 Entering Passive Mode (" +
                        std::to_string((ip >> 24) & 0xFF) + ',' +
                        std::to_string((ip >> 16) & 0xFF) + ',' +
                        std::to_string((ip >> 8) & 0xFF) + ',' +
                        std::to_string(ip & 0xFF) + ',' +
                        std::to_string((port >> 8) & 0xFF) + ',' +
                        std::to_string(port & 0xFF) + ").\r\n";
            
            message.sendMessage(clientSocket, response);
        }

        void activeMode(int clientSocket, std::vector<std::string> commands, int& activeSocket, sockaddr_in& activeAddress) {
            std::string data = commands[1];
            std::replace(data.begin(), data.end(), ',', ' ');
            std::istringstream stream(data);

            int h1, h2, h3, h4, p1, p2;
            stream >> h1 >> h2 >> h3 >> h4 >> p1 >> p2;

            std::string clientIP = std::to_string(h1) + "." +
                                   std::to_string(h2) + "." +
                                   std::to_string(h3) + "." +
                                   std::to_string(h4);

            int clientPort = (p1 * 256) + p2;

            LOG_INFO("Client IP: " + clientIP);
            LOG_INFO("Client Port: " + std::to_string(clientPort));

            activeSocket = socket(AF_INET, SOCK_STREAM, 0);

            if (activeSocket < 0) {
                perror("Error creating passive socket");
                exit(1);
            }

            activeAddress.sin_family = AF_INET;
            activeAddress.sin_port = htons(clientPort);
            inet_pton(AF_INET, clientIP.c_str(), &activeAddress.sin_addr);

            message.sendMessage(clientSocket, "227 Entering Active Mode.\r\n");
        }

        void get(int clientSocket, int& passiveSocket, int& activeSocket, sockaddr_in& activeAddress, const std::string& filePath) {
            // struct stat buffer;
            // if (stat(filePath.c_str(), &buffer) != 0) {
            //     message.sendMessage(clientSocket, "550 File not found.\r\n");
            //     return;
            // }

            LOG_INFO("File exists, sending file: " + filePath);

            if (passiveSocket != -1) {
                int dataSocket = accept(passiveSocket, NULL, NULL);
                if (dataSocket < 0) {
                    LOG_INFO("Error");
                    perror("Error accepting passive connection");
                    message.sendMessage(clientSocket, "451 Requested action aborted.\r\n");
                    return;
                }

                sendFile(filePath, dataSocket);
                message.sendMessage(clientSocket, "226 Transfer complete.\r\n");
                close(dataSocket);
            } else if (activeSocket != -1) {
                if (connect(activeSocket, (struct sockaddr*)&activeAddress, sizeof(activeAddress)) < 0) {
                    perror("Error connecting to client");
                    return;
                }
                sendFile(filePath, activeSocket);
                message.sendMessage(clientSocket, "226 Transfer complete.\r\n");
                close(activeSocket);
            } else {
                message.sendMessage(clientSocket, "425 Cannot use GET, use PASV or PORT first.\r\n");
            }
        }

        void stor(int clientSocket, int& passiveSocket, int& activeSocket, sockaddr_in& activeAddress, const std::string& filePath) {
            // struct stat buffer;
            // if (stat(filePath.c_str(), &buffer) != 0) {
            //     message.sendMessage(clientSocket, "550 File not found.\r\n");
            //     return;
            // }

            LOG_INFO("File exists, receiving file: " + filePath);

            if (passiveSocket != -1) {
                int dataSocket = accept(passiveSocket, NULL, NULL);
                if (dataSocket < 0) {
                    perror("Error accepting passive connection");
                    message.sendMessage(clientSocket, "451 Requested action aborted.\r\n");
                    return;
                }

                recevieFile(dataSocket, filePath);
                message.sendMessage(clientSocket, "226 Transfer complete\n");
                close(dataSocket);
            } else if (activeSocket != -1) {
                if (connect(activeSocket, (struct sockaddr*)&activeAddress, sizeof(activeAddress)) < 0) {
                    perror("Error connecting to client in active mode");
                    message.sendMessage(clientSocket, "425 Can't open data connection.\r\n");
                    return;
                }
                recevieFile(activeSocket, filePath);
                message.sendMessage(clientSocket, "226 Transfer complete\n");
                close(activeSocket);
            }else {
                message.sendMessage(clientSocket, "425 Cannot use GET, use PASV or PORT first.\r\n");
            }
        }

        void sendFile(std::string fileName, int socket) {
            // Open the file for reading
            std::ifstream file(fileName, std::ios::binary);
            if (!file.is_open()) {
                perror("Error opening file");
                close(socket);
                return;
            }

            LOG_INFO("File opened");
            
            // Send the file content to the client
            char buffer[1024];
            while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                size_t bytesRead = file.gcount();
                if (send(socket, buffer, bytesRead, 0) < 0) {
                    perror("Error sending data");
                    break;
                }
            }

            // Close the data connection and the file
            file.close();
        }

        void recevieFile(int socket, const std::string& filePath) {
            std::string filename = filePath;

            size_t lastSlashPos = filename.find_last_of('/');
            if (lastSlashPos != std::string::npos) {
                filename = filename.substr(lastSlashPos + 1);  
            }

            std::ofstream outputFile(filename, std::ios::binary); 
            if (!outputFile.is_open()) {
                message.sendMessage(socket, "550 Failed to open file for writing");
                return;
            }

            LOG_INFO("File opened: " + filename);
            
            char buffer[1024];
            ssize_t bytesRead;
            while ((bytesRead = recv(socket, buffer, sizeof(buffer), 0)) > 0) {
                outputFile.write(buffer, bytesRead);  
            }

            if (bytesRead < 0) {
                perror("Error receiving file data");
            }

            outputFile.close();
        }

    private:
        int passiveDataSocket;
        int activeDataSocket;

        FTPMessages message;
};

#endif
