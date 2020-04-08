#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "querySystem.hpp"

class ticketSystem {
private:
	std::map<std::string, int> corres = {{"trainID", 0}, {"stationNum", 1}, {"stations", 2},
		{"seatNum", 3}, {"prices", 4}, {"startTime", 5}, {"travelTimes", 6}, {"stopoverTimes", 7},
		{"saleDate", 8}, {"type", 9}, {"released", 10}};
	enum status{
		success, pending, refunded
	};
	
	database *c;
	std::string tableName;
	querySystem *query;
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
public:
	ticketSystem(database *_c, std::string _tableName, querySystem *_query) :
			c(_c), tableName(_tableName),  query(_query){
		std::string sql = "CREATE TABLE IF NOT EXISTS tickettable("
			"username varchar(255) PRIMARY KEY,"
			"orderCnt int," 
			"status int,"
			"trainID varchar(255),"
			"FROM varchar(255),"
			"LEAVING_TIME varchar(255),"
			"TO varchar(255),"
			"ARRIVING_TIME varchar(255),"
			"PRICE varchar(255)"
			"NUM varchar(4096);";
		c->executeTrans(sql);
	}
	
	int buy_ticket (std::string userName, std::string trainID, std::string date, std::string FROM,
			std::string TO, std::string queue) {
		std::ostringstream q;
		q << "INSERT INTO " << tableName << " (trainID,stationNum,seatNum,stations,prices,startTime,travelTimes,stopoverTimes,saleDate,type,released) "
			<< "VALUES (\'" << trainID << "\', " << stationNum << "," << seatNum << ", \'" << stations 
			<< "\', \'" << prices << "\', \'" << startTime << "\', \'" << travelTimes << "\', \'" 
			<< stopoverTimes << "\', \'" << saleDate << "\', \'" << type << "\'," << "false);";
		return c -> executeTrans(q.str());
	}
	
	std::pair<int, std::string> query_order(std::string userName) {
		auto info = getTicketInfo(userName);
		//if (info.first == -1) return std::make_pair(-1, "");

		std::ostringstream q;
		q << info.second[0][corres["trainID"]].as<std::string>() << " " 
			<< info.second[0][corres["type"]].as<std::string>() << "\n";
		
		int stationNum = info.second[0][corres["stationNum"]].as<int>();
		std::string startTime = info.second[0][corres["startTime"]].as<std::string>();
		
		moment curTime(date, startTime);
		int price = 0;
		q << stations[0] << " " << moment::emptyMoment << " -> " << curTime.toString() << " " << price << "\n";

		for (int i = 1; i < stationNum - 1; i++) {
			curTime += travelTimes[i - 1];
			q << stations[i] << " " << curTime.toString() << " -> ";
			curTime += stopoverTimes[i - 1];
			price += prices[i - 1];
			q << curTime.toString() << " " << price << "\n";
		}

		curTime += travelTimes[stationNum - 2];
		price += prices[stationNum - 2];
		q << stations[stationNum - 1] << " " << curTime.toString() << " -> " << moment::emptyMoment << " " << price << "\n";
		
		return std::make_pair(0, q.str());
	}
};
