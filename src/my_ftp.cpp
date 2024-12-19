#include "../include/server.hpp"

int main(int argc, char* argv[]) {
    
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <path>\n";
        exit(1);
    } 

    int port = std::stoi(argv[1]);

    Server server(port, argv[2]);
    
    server.startServer();

    return 0;
}