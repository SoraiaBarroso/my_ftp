#ifndef COMMAND_PARSER_HPP
#define COMMAND_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

enum eCommandType {PORT, LIST, QUIT, CD, PWD, RETR, USER, PASV, PASS, STOR};

class CommandParser {

    public:
        CommandParser();

        std::vector<std::string> parse(const std::string& str);

        void validate(std::vector<std::string>& commands);

        eCommandType getType(std::vector<std::string>& commands) const;

    private:
        std::map<std::string, eCommandType> commandTypeMap;

};

#endif
