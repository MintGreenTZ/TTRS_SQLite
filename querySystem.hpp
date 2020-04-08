#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include <moment>

class querySystem {
public:
	//saleDate eg. 06-01|08-17, success ensured
	void init_ticket (std::string trainID, std::string saleDate, std::string stationNum) {
		
	}
	
	//return value: (-1, "queue") or (-1, "") or (<price>, "")
	std::pair<int, std::string> buy_ticket (std::string userName, std::string trainID, std::string date,
			std::string FROM, std::string TO, std::string queue) {
		
	}
	
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_ticket(std::string start, std::string end, std::string date, std::string mode) {
		
	}
	
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_transfer(std::string start, std::string end, std::string date, std::string mode) {
		
	}
};
