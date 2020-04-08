#include <iostream>
#include <map>
#include "parser.hpp"
#include "database.hpp"
#include "userSystem.hpp"
#include "trainSystem.hpp"

std::map<std::string, int> CMDID = { {"add_user", 1}, {"login", 2}, {"logout", 3},
									 {"query_profile", 4}, {"modify_profile", 5},
									 {"add_train", 6},  {"release_train", 7}, {"query_train", 8}, {"delete_train", 9},
									 {"query_ticket", 10}, {"query_transfer", 11}, {"buy_ticket", 12},
									 {"query_order", 13}, {"refund_ticket", 14},
									 {"clean", 15}, {"exit", 16} };

// Compiler command : g++ main.cpp -lpqxx -lpq -o main
int main() {
    freopen("./testcases/dataTrain.in", "r", stdin);

    database *db = new database();
    userSystem *user = new userSystem(db, "usertable");
    querySystem *query = new querySystem(db, "querytable");
	trainSystem *train = new trainSystem(db, "traintable", query);
	ticketSystem *ticket = new ticketSystem(db, "tickettable", query);
	
    std::string c;
    while (getline(std::cin, c)) {
    	//std::cout << c << std::endl;
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
                ret = user -> login(args["u"], args["p"]);
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
			case 6: //add_train
				ret = train -> add_train(args["i"], args["n"], args["m"], args["s"], args["p"], args["x"], args["t"], args["o"], args["d"], args["y"]);
				std::cout << ret << std::endl;
				break;
			case 7: //release_train
				ret = train -> release_train(args["i"]);
				std::cout << ret << std::endl;
				break;
			case 8: //query_train
				res = train -> query_train(args["i"], args["d"]);
				std::cout << (res.first == -1 ? "-1" : res.second) << std::endl;
				break;
			case 9: //delete_train
				ret = train -> delete_train(args["i"]);
				std::cout << ret << std::endl;
				break;
			case 10: //query_ticket
				res = query -> query_ticket(args["s"], args["t"], args["d"]);
				std::cout << res.second << std::endl;
				break;
			case 11: //query_transfer
				res = query -> query_ticket(args["s"], args["t"], args["d"]);
				std::cout << res.second << std::endl;
				break;
			case 12: //buy_ticket
				ret = ticket -> buy_ticket(args["u"], args["i"], args["d"], args["n"], args["f"], args["t"], args["q"]);
				std::cout << ret << std::endl;
				break;
			case 13: //query_order
				res = ticket -> query_order(args["u"]);
				std::cout << res.second << std::endl;
				break;
			case 14: //refund_ticket
				ret = ticket -> refund_ticket(args["u"], args["n"]);
				std::cout << ret << std::endl;
				break;
            default:
                std::cerr << "Unknown command: " << t.first << std::endl;
        }
    }
    return 0;
}
