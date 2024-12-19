#include "../include/command_parser.hpp"
 
// Init with FTP commands available
CommandParser::CommandParser() {
    commandTypeMap = {
        {"QUIT", QUIT}, {"quit", QUIT}, 
        {"PWD", PWD},   {"pwd", PWD}, 
        {"PORT", PORT}, {"port", PORT},
        {"PASV", PASV}, {"pasv", PASV}, 
        {"LIST", LIST}, {"list", LIST}, 
        {"CD", CD},     {"cd", CD},     
        {"RETR", RETR}, {"retr", RETR},
        {"USER", USER}, {"user", USER},
        {"PASS", PASS}, {"pass", PASS},
        {"STOR", STOR}, {"stor", STOR},
    };
}

// Parse user input
std::vector<std::string> CommandParser::parse(const std::string& str) {
    std::vector<std::string> commands;
    commands.clear();

    size_t start = 0;
    size_t end = str.find(' ');

    // get command
    while (end != std::string::npos) {
        commands.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(' ', start);
    }

    commands.push_back(str.substr(start, end));

    validate(commands);

    return commands;
}

void CommandParser::validate(std::vector<std::string>& commands) {
    if (commands.empty()) {
        throw std::invalid_argument("Error: No command entered.");
    }

    if (commands.at(0).find_first_not_of(" \t") == std::string::npos) {
        throw std::invalid_argument("Error: Empty or invalid input.");
    }

    if (commandTypeMap.find(commands.at(0)) == commandTypeMap.end()) {
        std::cerr << "Invalid command: " << commands.at(0) << std::endl;
    }
}

eCommandType CommandParser::getType(std::vector<std::string>& commands) const {
    auto it = commandTypeMap.find(commands.at(0));

    if (it != commandTypeMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid command type: " + commands.at(0));
    }
}
