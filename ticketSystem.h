#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "database.hpp"

class querySystem;
class trainSystem;

class ticketSystem {
public:
	static std::map<std::string, int> corres;

private:
	enum status{
		success, pending, refunded
	};
	
	database *c, *cnt;
	std::string tableName;
	querySystem *query;
	trainSystem *train;
	int orderCnt;
private:
	std::pair<int, pqxx::result> getTicketInfo(std::string userName);

	std::pair<int, pqxx::result> getQueueInfo(std::string trainID);

	struct ticketInfo {
		std::string userName, trainID, date, num, fromSite, toSite;
	};

	void scanQueue(std::string trainID);

	bool checkFirst();
public:
	ticketSystem(database *_c, std::string _tableName, querySystem *_query, trainSystem *_train);

	~ticketSystem();
	
	int buy_ticket (std::string userName, std::string trainID, std::string date, std::string num,
			std::string FROM, std::string TO, std::string queue);
	
	std::pair<int, std::string> query_order(std::string userName);

	int refund_ticket(std::string userName, std::string string_n);
};
