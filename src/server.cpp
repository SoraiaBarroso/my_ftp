#include "../include/server.hpp"

// Handle user log in
bool Server::authentificateClient(int clientSocket) {
    const int maxRetries = 3;
    int attempt = 0;
    bool usernameReceived = false;

    while (attempt < maxRetries) {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Connection closed by client." << std::endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string userInput(buffer);

        // get command and command type        
        std::vector<std::string> command;
        try {
            command = parser.parse(buffer);
            commandType = parser.getType(command);
        } catch(const std::exception& e) {
            LOG_INFO(e.what());
            message.sendMessage(clientSocket, message.invalidCommand());
            continue;
        }

        switch (commandType) {
            case USER:
                if (usernameReceived) {
                    message.sendMessage(clientSocket, message.invalidPass());
                    break;
                }
                if (command.size() != 2) {
                    message.sendMessage(clientSocket, message.notEnoguhArgs());
                    break;
                }
                if (command[1] != "Anonymous") {
                    message.sendMessage(clientSocket, message.invalidUser());
                    attempt++;
                    break;
                }
                usernameReceived = true;
                message.sendMessage(clientSocket, message.userOK());
                break;
            case PASS:
                if (!usernameReceived) {
                    message.sendMessage(clientSocket, message.badSequenceCommand());
                    break;
                }
                if (command.size() == 1 || command[1].empty()) {
                    message.sendMessage(clientSocket, message.LoggedIn());
                    return true;
                } else {
                    message.sendMessage(clientSocket, message.invalidPass());
                    attempt++;
                    break;
                }
            case QUIT:
                LOG_INFO("QUIT requested");
                commandHandler.quit(clientSocket);
                break;
            default:
                message.sendMessage(clientSocket, "530 Please log in with USER and PASS.\n");
                attempt++;
                break;
        }
    }    

    message.sendMessage(clientSocket, message.tooManyFailedAttempts());
    return false;           
}

void Server::handleCommands(int clientSocket) {
    while (true) {
        char buffer[1024];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived <= 0) {
            std::cout << "Connection closed by client." << std::endl;
            break;
        }

        buffer[bytesReceived] = '\0';
        std::string userInput(buffer);

        // get command and command type
        std::vector<std::string> command;
        try {
            command = parser.parse(buffer);
            commandType = parser.getType(command);
        } catch(const std::exception& e) {
            LOG_INFO(e.what());
            message.sendMessage(clientSocket, message.invalidCommand());
            continue;
        }

        switch (commandType) {
            // ls [<remotedirectory>] [<localfile>]
            case LIST:
                LOG_INFO("LS requested");
                commandHandler.ls(clientSocket, command);
                break;
            case PWD:
                LOG_INFO("PWD requested");
                commandHandler.pwd(clientSocket);
                break;
            // cd <remotedirectory>
            case CD:
                LOG_INFO("CD requested");
                commandHandler.cd(clientSocket, command, originPath);
                break;
            // passive 
            case PASV:
                LOG_INFO("PASSIVE mode requested");
                commandHandler.passiveMode(clientSocket, passiveSocket, passiveAddress);
                break;
            // port 127,0,0,1,48,57
            case PORT:
                LOG_INFO("ACTIVE mode requested");
                commandHandler.activeMode(clientSocket, command, activeSocket, activeAddress);
                break;
            // only available in passive or active mode
            case RETR:
                LOG_INFO("GET requested");
                commandHandler.get(clientSocket, passiveSocket, activeSocket, activeAddress, command[1]);
                break;    
            // only available in passive or active mode
            case STOR:
                LOG_INFO("STOR requested"); 
                commandHandler.stor(clientSocket, passiveSocket, activeSocket, activeAddress, command[1]);
                break;
            case QUIT:
                LOG_INFO("QUIT requested");
                commandHandler.quit(clientSocket);
                break;
            default:
                break;
        }
    }   

    close(clientSocket);
    LOG_INFO("Client disconnected");
}