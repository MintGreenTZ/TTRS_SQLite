#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "querySystem.hpp"
#include "trainSystem.hpp"

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
	std::pair<int, pqxx::result> getTicketInfo(std::string userName) {
		std::ostringstream q;
		q << "SELECT * FROM " << tableName << " WHERE userName = \'" << userName << "\';";
		auto ret = c -> executeNonTrans(q.str());
		auto suc = ret.first;
		auto content = ret.second;
		if (suc == -1 || content.size() != 1)
			return std::make_pair(-1, content);
		else
			return std::make_pair(0, content);
	}

	std::pair<int, pqxx::result> getQueueInfo(std::string trainID) {
		std::ostringstream q;
		q << "SELECT * FROM " << tableName << " WHERE trainID = \'" << trainID << "\';";
		auto ret = c -> executeNonTrans(q.str());
		auto suc = ret.first;
		auto content = ret.second;
		if (suc == -1 || content.size() != 1)
			return std::make_pair(-1, content);
		else
			return std::make_pair(0, content);
	}

	struct ticketInfo {
		std::string userName, trainID, date, num, FROM, TO;
	}

	void scanQueue() {
		auto info = getQueueInfo(userName);
		std::vector<std::pair<int, ticketInfo>> allOrder;

		for (result::const_iterator it = info.second.begin(); it != info.second.end(); it++) {
			if ((it[corres["status"]].as<int>() != pending) continue;
			std::ostringstream q;
			ticketInfo info = {it[corres["userName"]].as<std::string>(),
							   it[corres["trainID"]].as<std::string>(),
							   it[corres["date"]].as<std::string>(),
							   it[corres["num"]].as<std::string>(),
							   it[corres["FROM"]].as<std::string>(),
							   it[corres["TO"]].as<std::string>()};
			allOrder.push_back(std::make_pair<int, ticketInfo>(it[corres["orderCnt"]], info));
		}
		std::sort(allOrder.begin(), allOrder.end());

		for (auto it = allOrder.begin(); it != allOrder.end(); it++) {
			if (query -> buy_ticket(it->second.userName, it->second.trainID, it->second.date, 
					it->second.num, it->second.FROM, it->second.TO).first != -1) {
				std::ostringstream q;
				q << "UPDATE " << tableName << " SET status = 0 WHERE userName = \'"
					<< it->second.userName << "\' AND orderCnt = " << it->first << ";";
				c -> executeTrans(q.str());
			}
		}
	}

	bool checkFirst() {
        std::string q = "select true from " + tableName + " limit 1;";
        auto ret = c -> executeNonTrans(q);
        return ret.second.size() == 0;
    }
public:
	ticketSystem(database *_c, std::string _tableName, querySystem *_query, trainSystem *_train) :
			c(_c), cnt(_c), tableName(_tableName), query(_query), train(_train){
		std::string sql = "CREATE TABLE IF NOT EXISTS tickettable("
			"userName varchar(255) PRIMARY KEY,"
			"orderCnt int," 
			"status int,"
			"trainID varchar(255),"
			"FROM varchar(255),"
			"LEAVING_TIME varchar(255),"
			"TO varchar(255),"
			"ARRIVING_TIME varchar(255),"
			"price varchar(255)"
			"num varchar(255)"
			"date varchar(255))";
		c -> executeTrans(sql);
		if (checkFirst()) {
			cnt -> executeTrans("CREATE TABLE IF NOT EXISTS cnttable(cnt int PRIMARY KEY);");
			cnt -> executeTrans("INSERT INTO (cnt) VALUES (0);");
			orderCnt = 0;
		} else {
			auto ret = cnt -> executeNonTrans("SELECT * FROM " + tableName + " ;");
			orderCnt = ret.second[0].as<int>();
		}
	}

	~ticketSystem() {
		cnt -> executeTrans("UPDATE cnttable SET cnt = " + cnt + ";");
	}
	
	int buy_ticket (std::string userName, std::string trainID, std::string date, std::string num,
			std::string FROM, std::string TO, std::string queue) {
		std::pair<int, std::string> res = query -> buy_ticket(userName, trainID, date, FROM, TO, queue);
		if (res.first == -1) return -1;
		std::ostringstream q;
		if (res.second == "queue") { // inqueue
			std::pair<std::string, std::string> times = train -> findTime(trainID, date, FROM, TO);
			q << "INSERT INTO " << tableName << " (userName,orderCnt,status,trainID,FROM,LEAVING_TIME,TO,ARRIVING_TIME,price,num,date) "
			<< "VALUES (\'" << userName << "\', " << orderCnt++ << "," << pending << ", \'" << trainID 
			<< "\', \'" << FROM << "\', \'" << times.first << "\', \'" << TO << "\', \'" << times.second
			<< "\', " << price << "," << num << ", \'" date << "\');";
			c -> executeTrans(q.str());
			return -2;
		} else { //success
			std::pair<std::string, std::string> times = train -> findTime(trainID, date, FROM, TO);
			q << "INSERT INTO " << tableName << " (userName,orderCnt,status,trainID,FROM,LEAVING_TIME,TO,ARRIVING_TIME,price,num) "
			<< "VALUES (\'" << userName << "\', " << orderCnt++ << "," << success << ", \'" << trainID 
			<< "\', \'" << FROM << "\', \'" << times.first << "\', \'" << TO << "\', \'" << times.second
			<< "\', " << price << "," << num << ", \'" date << "\');";
			c -> executeTrans(q.str());
			return res.first;
		}
	}
	
	std::pair<int, std::string> query_order(std::string userName) {
		auto info = getTicketInfo(userName);
		std::vector<std::pair<int, std::string>> allOrder;

		for (result::const_iterator it = info.second.begin(); it != info.second.end(); it++) {
			std::ostringstream q;
			switch (it[corres["status"]].as<int>()) {
				case success: 	q << "[success] ";	break;
				case pending: 	q << "[pending] ";	break;
				case refunded:	q << "[refund] ";	break;
			}
			q << it[corres["trainID"]].as<std::string>() << " ";
			q << it[corres["FROM"]].as<std::string>() << " ";
			q << it[corres["LEAVING_TIME"]].as<std::string>() << " -> ";
			q << it[corres["TO"]].as<std::string>() << " ";
			q << it[corres["ARRIVING_TIME"]].as<std::string>() << " ";
			q << it[corres["price"]].as<int>() << " ";
			q << it[corres["num"]].as<int>() << "\n";
			allOrder.push_back(std::make_pair<int, std::string>(it[corres["orderCnt"]], q.str()));
		}
		std::sort(allOrder.begin(), allOrder.end());
		std::ostringstream q;
		q << allOrder.size() << "\n";
		for (auto it = allOrder.begin(); it != allOrder.end(); it++)
			q << *it;
		return std::make_pair(0, q.str());
	}

	int refund_ticket(std::string userName, std::string string_n) {
		int n = std::stoi(string_n);
		auto info = getTicketInfo(userName);
		int cnt = 1;
		for (result::const_iterator it = info.second.begin(); it != info.second.end(); it++, cnt++) {
			if (cnt == n) {
				if (it[corres["status"]].as<int>() == success) {
					std::ostringstream q;
					q << "UPDATE " << tableName << " SET status = 2 WHERE userName = \'" << userName
					<< "\' AND orderCnt = " << it[corres["orderCnt"]].as<int>() << ";";
					c -> executeNonTrans(q.str());
					query -> add_ticket(it[corres["trainID"]].as<std::string>(), it[corres["date"]].as<std::string>(),
						it[corres["num"]].as<std::string>(), it[corres["FROM"]].as<std::string>(),
						it[corres["TO"]].as<std::string>());
					scanQueue(trainID);
					return 0;
				} else
					return -1;
			}
		}
		return -1;
	}
};
