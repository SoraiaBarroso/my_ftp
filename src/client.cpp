#include <iostream>
#include <string>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <regex>
#include <sstream>
#include <iomanip>

class FTPClient {
public:
    FTPClient(const std::string& serverIp, int serverPort)
        : serverIp(serverIp), serverPort(serverPort), controlSocket(-1), passiveSocket(-1) {}

    bool connectToServer() {
        controlSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (controlSocket < 0) {
            perror("Error creating control socket");
            return false;
        }

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIp.c_str(), &serverAddress.sin_addr) <= 0) {
            perror("Invalid server IP address");
            close(controlSocket);
            return false;
        }

        if (connect(controlSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
            perror("Error connecting to server");
            close(controlSocket);
            return false;
        }

        std::cout << "Connected to server on " << serverIp << ":" << serverPort << std::endl;
        return true;
    }

    void closeConnection() {
        if (controlSocket != -1) {
            close(controlSocket);
        }
        if (passiveSocket != -1) {
            close(passiveSocket);
        }
        if (activeSocket != -1) {
            close(activeSocket);
        }
    }
    
    void sendMessage(const std::string& message) {
        ssize_t bytesSent = send(controlSocket, message.c_str(), message.length(), 0);
        if (bytesSent < 0) {
            perror("Error sending message");
        }
    }

    std::string receiveMessage() {
        char buffer[1024];
        ssize_t bytesRead = recv(controlSocket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0) {
            perror("Error receiving message");
            return "";
        }
        return std::string(buffer, bytesRead);
    }

    // Client send their ip and a random port for the server to connect
    void handleActiveMode(std::string input) {
        std::string response = receiveMessage();
        std::cout << response;

        std::regex portRegex("PORT (\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)", std::regex::icase);
        std::smatch match;

        if (std::regex_match(input, match, portRegex)) {
            // Extract the client IP address
            std::string ip = match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
            
            // Extract and calculate the client port number
            int port1 = std::stoi(match[5].str());
            int port2 = std::stoi(match[6].str());
            int port = (port1 * 256) + port2;

            activeSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (activeSocket < 0) {
                perror("Error creating socket");
                return;
            }

            sockaddr_in clientAddress;
            clientAddress.sin_family = AF_INET;
            clientAddress.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &clientAddress.sin_addr);

            if (bind(activeSocket, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0) {
                perror("Error binding to port");
                return;
            }

            if (listen(activeSocket, 1) < 0) {
                perror("Error listening on port");
                return;
            }

            std::cout << "Listening on port " << port << " for incoming data connection..." << std::endl;

            while (true) {
                std::cout << "ftp> ";

                std::string input;
                std::getline(std::cin, input);

                if (input.find("RETR") == 0 || input.find("retr") == 0) {
                    dowloadFileActive(input);
                    return;
                } else if (input.find("STOR") == 0 || input.find("stor") == 0) {
                    sendFileActive(input);
                    return;
                }

                sendMessage(input);

                std::string serverResponse;
                serverResponse = receiveMessage();
                std::cout << serverResponse;
            }
        } 
    }

    // Client receives an ip and a port
    void handlePassiveMode() {
        std::string response = receiveMessage();
        std::cout << response;

        std::regex passiveModeRegex("227 Entering Passive Mode \\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\)");
        std::smatch match;

        if (std::regex_search(response, match, passiveModeRegex)) {
            std::string ip = match[1].str() + "." + match[2].str() + "." + match[3].str() + "." + match[4].str();
            int p1 = std::stoi(match[5].str());
            int p2 = std::stoi(match[6].str());
          
            int port = (p1 * 256) + p2;

            // Now connect to the passive mode port
            passiveSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (passiveSocket < 0) {
                perror("Error creating passive socket");
                return;
            }

            sockaddr_in passiveServerAddress;
            passiveServerAddress.sin_family = AF_INET;
            passiveServerAddress.sin_port = htons(port);

            if (inet_pton(AF_INET, ip.c_str(), &passiveServerAddress.sin_addr) <= 0) {
                perror("Invalid passive mode server IP address");
                close(passiveSocket);
                return;
            }

            if (connect(passiveSocket, (struct sockaddr*)&passiveServerAddress, sizeof(passiveServerAddress)) < 0) {
                perror("Error connecting to passive mode server");
                close(passiveSocket);
                return;
            }

            while (true) {
                std::cout << "ftp> ";

                std::string input;
                std::getline(std::cin, input);

                if (input.find("RETR") == 0 || input.find("retr") == 0) {
                    dowloadFile(input);
                    return;
                } else if (input.find("STOR") == 0 || input.find("stor") == 0) {
                    sendFile(input);
                    return;
                }

                sendMessage(input);

                std::string serverResponse;
                serverResponse = receiveMessage();
                std::cout << serverResponse;
            }
        }
    }

    void sendFileActive(std::string input) {
        std::string filename = input.substr(5); 

        std::ifstream inputFile(filename, std::ios::binary);
        if (!inputFile) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }

        std::cout << "sending file";

        sendMessage(input);

        sockaddr_in serverAddress;
        socklen_t serverAddressLen = sizeof(serverAddress);
        int dataSocket = accept(activeSocket, (struct sockaddr*)&serverAddress, &serverAddressLen);
        if (dataSocket < 0) {
            perror("Error accepting server connection");
            return;
        }

        char buffer[1024];
        while (inputFile.read(buffer, sizeof(buffer)) || inputFile.gcount() > 0) {
            size_t bytesRead = inputFile.gcount();
            if (send(dataSocket, buffer, bytesRead, 0) < 0) {
                perror("Error sending file data");
                break;
            }
        }

        if (inputFile.eof()) {
            std::cout << "File sent successfully: " << filename << std::endl;
        } else {
            std::cerr << "Error reading file: " << filename << std::endl;
        }

    
        inputFile.close();
        close(activeSocket); 
        close(dataSocket);
        activeSocket = -1; 
    }

    void sendFile(std::string input) {
        std::string filename = input.substr(5); 

        std::ifstream inputFile(filename, std::ios::binary);
        if (!inputFile) {
            std::cerr << "Error opening file for reading: " << filename << std::endl;
            return;
        }

        sendMessage(input);

        char buffer[1024];
        while (inputFile.read(buffer, sizeof(buffer)) || inputFile.gcount() > 0) {
            size_t bytesRead = inputFile.gcount();
            if (send(passiveSocket, buffer, bytesRead, 0) < 0) {
                perror("Error sending data");
                break;
            }
        }

        inputFile.close();
        close(passiveSocket);
        passiveSocket = -1;
    }

    void dowloadFileActive(std::string input) {
        std::string filename = input.substr(4);

        // Get only the base filename
        size_t lastSlashPos = filename.find_last_of('/');
        if (lastSlashPos != std::string::npos) {
            filename = filename.substr(lastSlashPos + 1);  
        }
        
        sendMessage(input);

        sockaddr_in serverAddress;
        socklen_t serverAddressLen = sizeof(serverAddress);
        int dataSocket = accept(activeSocket, (struct sockaddr*)&serverAddress, &serverAddressLen);
        if (dataSocket < 0) {
            perror("Error accepting server connection");
            return;
        }

        // Receive file data over the passive socket
        std::ofstream outputFile(filename, std::ios::binary);
        if (!outputFile) {
            std::cerr << "Error opening file for writing" << std::endl;
            close(activeSocket);
            return;
        } 

        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = recv(dataSocket, buffer, sizeof(buffer), 0)) > 0) {
            outputFile.write(buffer, bytesRead);
        }

        if (bytesRead < 0) {
            perror("Error receiving file data");
        } 

        outputFile.close();

        close(dataSocket);
        close(activeSocket);
        activeSocket = -1;
    }

    void dowloadFile(std::string input) {
        std::string filename = input.substr(4);

        size_t lastSlashPos = filename.find_last_of('/');
        if (lastSlashPos != std::string::npos) {
            filename = filename.substr(lastSlashPos + 1);  
        }
        
        sendMessage(input);

        std::ofstream outputFile(filename, std::ios::binary);
        if (!outputFile) {
            std::cerr << "Error opening file for writing" << std::endl;
            close(passiveSocket);
            return;
        } 

        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = recv(passiveSocket, buffer, sizeof(buffer), 0)) > 0) {
            outputFile.write(buffer, bytesRead);
        }

        if (bytesRead < 0) {
            perror("Error receiving file data");
        } 

        outputFile.close();

        close(passiveSocket);
        passiveSocket = -1;
    }


private:
    std::string serverIp;
    int serverPort;
    int controlSocket;
    int passiveSocket = -1;
    int activeSocket = -1;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        exit(1);
    } 

    bool loggedIn = false;
    int port = std::stoi(argv[1]);

    FTPClient client("127.0.0.1", port);  

    if (client.connectToServer()) {
        std::string serverResponse;

        while (true) { 
            serverResponse = client.receiveMessage();

            if (serverResponse.empty()) {
                std::cout << "Connection lost or no data received." << std::endl;
                break; 
            }

            std::cout << serverResponse;

            if (serverResponse == "230 User logged in, proceed.\r\n") {
                loggedIn = true;
            }

            while (true) {
                std::cout << "ftp> ";
                std::string input;
                std::getline(std::cin, input);

                // Go back to 'ftp>' prompt
                if (input.empty()) {
                    continue; 
                }

                client.sendMessage(input);

                if (input == "QUIT" || input == "quit") {
                    client.closeConnection();
                    std::cout << "Exiting...";
                    exit(0); 
                }

                if (loggedIn) {
                    if (input == "PASV" || input == "pasv") {
                        client.handlePassiveMode();
                    } 
                    else if (input.substr(0, 4) == "PORT" || input.substr(0, 4) == "port") {
                        client.handleActiveMode(input);
                    }
                } else {
                    break;
                }

                break;
            }
        }
        client.closeConnection();
    }

    return 0;
}
