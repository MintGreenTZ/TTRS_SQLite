#include <iostream>
#include <map>
#include "parser.hpp"
#include "database.hpp"
#include "userSystem.hpp"

std::map<std::string, int> CMDID = { {"add_user", 1}, {"login", 2} };

// Compiler command : g++ main.cpp -lpqxx -lpq -o main
int main() {
    freopen("./testcases/data.in", "r", stdin);

    database *db = new database();
    userSystem *user = new userSystem(db, "usertable");

    std::string c;
    while (getline(std::cin, c)) {
        auto t = parser::parse(c);
        if (t.first == "") break;

        auto cmd = CMDID[t.first];
        auto args = t.second;
        
        int ret;
        switch (cmd) {
            case 1: //add_user
                ret = user -> addUser(args["u"], args["p"], args["n"], args["m"], args["g"]);
                break;
            case 2: //login
                ret = user -> login(args["u"], args["p"]);
                break;
            case 3: //logout
                ret = user -> login(args["u"]);
                break;
            default:
                std::cerr << "Unknown command: " << t.first << std::endl;
        }
        std::cout << ret << std::endl;
    }
    return 0;
}