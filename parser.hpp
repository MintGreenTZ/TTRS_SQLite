#pragma once
#include <utility>
#include <string>
#include <map>

class parser {
public:
    static std::string getNext(std::string &str) {
        std::string ret = str;
        auto p = str.find(" ");
        if (p != std::string::npos) ret = str.substr(0, p), str = str.substr(p + 1, str.length() - 1 - p);
        else str = "";
        return ret;
    }

    static std::pair<std::string, std::map<std::string, std::string>> parse(std::string str) {
        while (str.length() >= 1 && (str[str.length() - 1] == '\r' || str[str.length() - 1] == '\n' || str[str.length() - 1] == ' ')) str = str.substr(0, str.length() - 1);

        std::string cmd = getNext(str);
        
        std::map<std::string, std::string> p;

        while (str != "") {            
            auto key = getNext(str);
            auto argument = getNext(str);
            p[key.substr(1, key.length() - 1)] = argument;
        }
        return std::make_pair(cmd, p);
    }

    static void replaceByDefault(std::string &val, std::string d) {
        val = (val == "") ? d : val;
    }
};