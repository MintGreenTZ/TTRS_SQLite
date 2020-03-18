#include <iostream>
#include <map>
#include "parser.hpp"
#include "database.hpp"
#include "userSystem.hpp"

std::map<std::string, int> CMDID = { {"add_user", 1}, {"login", 2}, {"logout", 3}, 
                                     {"query_profile", 4},{"modify_profile", 5} };

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
        std::pair<int, std::string> res;

        switch (cmd) {
            case 1: //add_user
                ret = user -> addUser(args["c"], args["u"], args["p"], args["n"], args["m"], args["g"]);
                std::cout << ret << std::endl;
                break;
            case 2: //login
                ret = user->login(args["u"], args["p"]);
                std::cout << ret << std::endl;
                break;
            case 3: //logout
                ret = user -> logout(args["u"]);
                std::cout << ret << std::endl;
                break;
            case 4: //query_profile
                res = user -> query_profile(args["c"], args["u"]);
                std::cout << (res.first == -1 ? "-1" : res.second) << std::endl;
                break;
            case 5: //modify_profile
                res = user -> modify_profile(args["c"], args["u"], args["p"], args["n"], args["m"], args["g"]);
                std::cout << (res.first == -1 ? "-1" : res.second) << std::endl;
                break;
            default:
                std::cerr << "Unknown command: " << t.first << std::endl;
        }
    }
    return 0;
}