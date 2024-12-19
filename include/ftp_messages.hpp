#ifndef INFO_MESSAGES_HPP
#define INFO_MESSAGES_HPP

    #include <sys/socket.h>
    #include <iostream>
    #include <string>

    #define LOG_INFO(msg)  std::cout << "[INFO]> "  << msg << std::endl
    #define LOG_ERROR(msg) std::cerr << "[ERROR]> " << msg << std::endl

    class FTPMessages {
        public:
            static std::string notLoggedIn() {
                return "530 Not logged in, please try again.\r\n";
            }

            static std::string serviceReady() {
                return "220 Service ready for new user. Please Log In\r\n";
            }

            static std::string tooManyFailedAttempts() {
                return "530 Too many failed attempts. Disconnecting...\r\n";
            }

            static std::string LoggedIn() {
                return "230 User logged in, proceed.\r\n";
            }
            
            static std::string badSequenceCommand() {
                return "503 Bad sequence of commands. Use USER first.\n";
            }

            static std::string invalidUser() {
                return "530 Invalid username.\r\n";
            }

            static std::string invalidPass() {
                return "530 Invalid password.\r\n";
            }

            static std::string invalidCommand() {
                return "500 Empty or invalid input. Please type a valid command.\r\n";
            }

            static std::string userOK() {
                return "331 User name okay, need password.\r\n";
            }
            
            static std::string notEnoguhArgs() {
                return "501 Wrong arguments\r\n";
            }
            
            void sendMessage(int clientSocket, const std::string& message) {
                int bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
                if (bytesSent == -1) {
                    std::cerr << "Error sending message\n";
                }
                fflush(stdout);  
            }
    };
#endif
