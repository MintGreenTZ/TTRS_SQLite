#include "querySystem.h"
#include "trainSystem.h"
#include "ticketSystem.h"
#include "userSystem.h"

std::pair<int, pqxx::result> ticketSystem::getTicketInfo(std::string userName) {
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

std::pair<int, pqxx::result> ticketSystem::getQueueInfo(std::string trainID) {
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
	std::string userName, trainID, date, num, fromSite, toSite;
};

void ticketSystem::scanQueue(std::string trainID) {
	auto info = getQueueInfo(trainID);
	std::vector<std::pair<int, ticketInfo>> allOrder;

	for (pqxx::result::const_iterator it = info.second.begin(); it != info.second.end(); it++) {
		if (it[corres["status"]].as<int>() != pending) continue;
		std::ostringstream q;
		ticketInfo info = {it[corres["userName"]].as<std::string>(),
							it[corres["trainID"]].as<std::string>(),
							it[corres["date"]].as<std::string>(),
							it[corres["num"]].as<std::string>(),
							it[corres["fromSite"]].as<std::string>(),
							it[corres["toSite"]].as<std::string>()};
		allOrder.push_back(std::make_pair(it[corres["orderCnt"]].as<int>(), info));
	}
	std::sort(allOrder.begin(), allOrder.end(),[](auto a, auto b) -> bool { return a.first < b.first; });

	bool alterSuccess = false;
	std::vector<std::pair<int, std::string> > result;
	std::pair<int, std::string> res;

	for (auto it = allOrder.begin(); it != allOrder.end(); it++) {
		res = query->buy_ticket("", it->second.trainID, it->second.date,
			it->second.num, it->second.fromSite, it->second.toSite, "false");
		result.push_back(res);
		if (res.first != -1) {
			std::ostringstream q;
			q << "UPDATE " << tableName << " SET status = 0 WHERE userName = \'"
				<< it->second.userName << "\' AND orderCnt = " << it->first << ";";
			c -> executeTrans(q.str());
			std::cout << "Alternate " << it->second.userName << " " << it->second.trainID << " " << it->second.date << " " << it->second.num << " " << it->second.fromSite << " " << it->second.toSite << std::endl;
			alterSuccess = true;
		}
	}
	if (alterSuccess) {
		auto resultIt = result.begin();
		for (auto it = allOrder.begin(); it != allOrder.end(); it++, resultIt++) {
			std::cout << "List " << it->second.userName << " " << it->second.trainID << " " << it->second.date << " " << it->second.num << " " << it->second.fromSite << " " << it->second.toSite << std::endl;
			std::cout << resultIt->first << " " << resultIt->second << std::endl;
		}
	}
}

bool ticketSystem::checkFirst() {
	std::string q = "select true from " + tableName + " limit 1;";
	auto ret = c -> executeNonTrans(q);
	return ret.second.size() == 0;
}

ticketSystem::ticketSystem(database *_c, std::string _tableName, querySystem *_query, trainSystem *_train, userSystem *_user) :
		c(_c), cnt(_c), tableName(_tableName), query(_query), train(_train), user(_user) {
	std::string sql = "CREATE TABLE IF NOT EXISTS tickettable("
		"userName varchar(255),"
		"orderCnt int," 
		"status int,"
		"trainID varchar(255),"
		"fromSite varchar(255),"
		"LEAVING_TIME varchar(255),"
		"toSite varchar(255),"
		"ARRIVING_TIME varchar(255),"
		"price varchar(255),"
		"num int,"
		"date varchar(255));";
	c -> executeTrans(sql);
	if (checkFirst()) {
		cnt -> executeTrans("CREATE TABLE IF NOT EXISTS cnttable(cnt int PRIMARY KEY);");
		cnt -> executeTrans("INSERT INTO cnttable (cnt) VALUES (0);");
		orderCnt = 0;
	} else {
		auto ret = cnt -> executeNonTrans("SELECT * FROM " + tableName + " ;");
		orderCnt = ret.second[0][0].as<int>();
	}
}

ticketSystem::~ticketSystem() {
	cnt -> executeTrans("UPDATE cnttable SET cnt = " + std::to_string(orderCnt) + ";");
}

int ticketSystem::buy_ticket (std::string userName, std::string trainID, std::string date, std::string num,
		std::string fromSite, std::string toSite, std::string queue) {
	std::pair<int, std::string> res = query -> buy_ticket(userName, trainID, date, num, fromSite, toSite, queue);
	if (res.first == -1) return -1;
	std::ostringstream q;
	std::pair<std::string, std::string> times = train -> findTime(trainID, date, fromSite, toSite);
	//std::cout << "[times] " << times.first << " " << times.second << std::endl;
	if (res.second == "queue") { // inqueue
		q << "INSERT INTO " << tableName << " (userName,orderCnt,status,trainID,fromSite,LEAVING_TIME,toSite,ARRIVING_TIME,price,num,date) "
		<< "VALUES (\'" << userName << "\', " << orderCnt++ << "," << pending << ", \'" << trainID 
		<< "\', \'" << fromSite << "\', \'" << times.first << "\', \'" << toSite << "\', \'" << times.second
		<< "\', " << int(res.first / atoi(num.c_str())) << "," << num << ", \'" << date << "\');";
		// std::cout << "[Success Queue!]" << std::endl;
		// std::cout << "!!!" << q.str() << std::endl;
		c -> executeTrans(q.str());
		return -2;
	} else { //success
		q << "INSERT INTO " << tableName << " (userName,orderCnt,status,trainID,fromSite,LEAVING_TIME,toSite,ARRIVING_TIME,price,num,date) "
		<< "VALUES (\'" << userName << "\', " << orderCnt++ << "," << success << ", \'" << trainID 
		<< "\', \'" << fromSite << "\', \'" << times.first << "\', \'" << toSite << "\', \'" << times.second
		<< "\', " << int(res.first / atoi(num.c_str())) << "," << num << ", \'" << date << "\');";
		// std::cout << "[Success Insert!]" << std::endl;
		// std::cout << "!!!" << q.str() << std::endl;
		c -> executeTrans(q.str());
		return res.first;
	}
}

std::pair<int, std::string> ticketSystem::query_order(std::string userName) {
	if (!user->checkUser(userName)) return std::make_pair(0, "-1\n");

	auto info = getTicketInfo(userName);
	std::vector<std::pair<int, std::string>> allOrder;

	for (pqxx::result::const_iterator it = info.second.begin(); it != info.second.end(); it++) {
		std::ostringstream q;
		switch (it[corres["status"]].as<int>()) {
			case success: 	q << "[success] ";	break;
			case pending: 	q << "[pending] ";	break;
			case refunded:	q << "[refunded] ";	break;
		}
		q << it[corres["trainID"]].as<std::string>() << " ";
		q << it[corres["fromSite"]].as<std::string>() << " ";
		q << it[corres["LEAVING_TIME"]].as<std::string>() << " -> ";
		q << it[corres["toSite"]].as<std::string>() << " ";
		q << it[corres["ARRIVING_TIME"]].as<std::string>() << " ";
		q << it[corres["price"]].as<int>() << " ";
		q << it[corres["num"]].as<int>() << "\n";
		// std::cout<< "WTF IS THIS" << it[corres["ARRIVING_TIME"]].as<std::string>() << std::endl;
		allOrder.push_back(std::make_pair<int, std::string>(-it[corres["orderCnt"]].as<int>(), q.str()));
	}
	std::sort(allOrder.begin(), allOrder.end());
	/*std::cout << "orderCnt ";
	for (int i = 0; i < allOrder.size(); i++)
		std::cout << allOrder[i].first << " ";
	std::cout << std::endl;*/
	std::ostringstream q;
	q << allOrder.size() << "\n";
	for (auto it = allOrder.begin(); it != allOrder.end(); it++)
		q << it -> second;
	return std::make_pair(0, q.str());
}

int ticketSystem::refund_ticket(std::string userName, std::string string_n) {
	//if (userName == "Shining") std::cout << "Shining is refunding!" << std::endl;
	if (!user -> checkUser(userName)) return -1;
	
	int n = std::stoi(string_n);
	auto info = getTicketInfo(userName);
	int cnt = 1;
	n = info.second.size() - n + 1;
	for (pqxx::result::const_iterator it = info.second.begin(); it != info.second.end(); it++, cnt++) {
		if (cnt == n) {
			if (it[corres["status"]].as<int>() == success || it[corres["status"]].as<int>() == pending) {
				std::ostringstream q;
				q << "UPDATE " << tableName << " SET status = 2 WHERE userName = \'" << userName
				<< "\' AND orderCnt = " << it[corres["orderCnt"]].as<int>() << ";";
				c -> executeNonTrans(q.str());
				// std::cout << "[DATE] " << it[corres["date"]].as<std::string>() << std::endl;
				if (it[corres["status"]].as<int>() == success) {
					bool ret = query->add_ticket(userName, it[corres["trainID"]].as<std::string>(), it[corres["date"]].as<std::string>(),
						it[corres["num"]].as<std::string>(), it[corres["fromSite"]].as<std::string>(),
						it[corres["toSite"]].as<std::string>());
					if (!ret) return -1;
					scanQueue(it[corres["trainID"]].as<std::string>());
				}
				// std::cout << "[Refund end]" << std::endl;
				return 0;
			} else
				return -1;
		}
	}
	return -1;
}

