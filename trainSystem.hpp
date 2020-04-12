#pragma once

#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "arrayParser.hpp"
#include "moment.hpp"
#include "querySystem.hpp"

class trainSystem {
public:
	static std::map<std::string, int> corres;
private:
	database *c, *city2trainID;
	std::string tableName;
	querySystem *query;
public:
	std::pair<int, pqxx::result> getTrainInfo(std::string trainID) {
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
public:
	trainSystem(database *_c, std::string _tableName, querySystem *_query) :
			c(_c), city2trainID(_c), tableName(_tableName), query(_query) {
		std::string sql = "CREATE TABLE IF NOT EXISTS traintable("
			"trainID varchar(255) PRIMARY KEY,"
			"stationNum int,"
			"stations varchar(4096)," // stationNum
			"seatNum int,"
			"prices varchar(4096)," // stationNum - 1				0|contents
			"startTime varchar(255),"
			"travelTimes varchar(4096)," // stationNum - 1			0|contents
			"stopoverTimes varchar(4096)," // stationNum - 2		0|contents|0
			"saleDate varchar(255),"
			"type varchar(1),"
			"released boolean );";
		c -> executeTrans(sql);
		sql = "CREATE TABLE IF NOT EXISTS city2trainIDtable("
			"city varchar(255) PRIMARY KEY,"
			"trainID varchar(255);";
		city2trainID -> executeTrans(sql);
	}

	int add_train(std::string trainID, std::string stationNum, std::string seatNum, std::string stations, 
				std::string prices, std::string startTime, std::string travelTimes, 
				std::string stopoverTimes, std::string saleDate, std::string type) {
		std::ostringstream q;
		prices = "0|" + prices;
		travelTimes = "0|" + travelTimes;
		stopoverTimes = "0|" + stopoverTimes + "|0";
		q << "INSERT INTO " << tableName << " (trainID,stationNum,seatNum,stations,prices,startTime,travelTimes,stopoverTimes,saleDate,type,released) "
			<< "VALUES (\'" << trainID << "\', " << stationNum << "," << seatNum << ", \'" << stations 
			<< "\', \'" << prices << "\', \'" << startTime << "\', \'" << travelTimes << "\', \'" 
			<< stopoverTimes << "\', \'" << saleDate << "\', \'" << type << "\'," << "false);";
		return c -> executeTrans(q.str());
	}

	std::pair<int, std::string> query_train(std::string trainID, std::string date) {
		auto info = getTrainInfo(trainID);
		if (info.first == -1 || info.second[0][corres["released"]].as<bool>() == false) return std::make_pair(-1, "");
		std::vector<std::string> saleDate = arrayParser<std::string>::parse(info.second[0][corres["saleDate"]].as<std::string>());
		if (date < saleDate[0] || saleDate[1] < date) return std::make_pair(-1, "");
		
		std::ostringstream q;
		q << info.second[0][corres["trainID"]].as<std::string>() << " " 
			<< info.second[0][corres["type"]].as<std::string>() << "\n";
		
		int stationNum = info.second[0][corres["stationNum"]].as<int>();
		std::vector<std::string> stations = arrayParser<std::string>::parse(info.second[0][corres["stations"]].as<std::string>());
		std::vector<int> prices = arrayParser<int>::parse(info.second[0][corres["prices"]].as<std::string>());
		std::vector<int> travelTimes = arrayParser<int>::parse(info.second[0][corres["travelTimes"]].as<std::string>());
		std::vector<int> stopoverTimes = arrayParser<int>::parse(info.second[0][corres["stopoverTimes"]].as<std::string>());
		std::string startTime = info.second[0][corres["startTime"]].as<std::string>();
		
		moment curTime(date, startTime);
		int price = 0;
		q << stations[0] << " " << moment::emptyMoment << " -> " << curTime.toString() << " " << price << "\n";

		for (int i = 1; i < stationNum - 1; i++) {
			curTime += travelTimes[i];
			q << stations[i] << " " << curTime.toString() << " -> ";
			curTime += stopoverTimes[i];
			price += prices[i];
			q << curTime.toString() << " " << price << "\n";
		}

		curTime += travelTimes[stationNum - 1];
		price += prices[stationNum - 1];
		q << stations[stationNum - 1] << " " << curTime.toString() << " -> " << moment::emptyMoment << " " << price << "\n";
		
		return std::make_pair(0, q.str());
	}

	int release_train(std::string trainID) {
		auto info = getTrainInfo(trainID);
		if (info.first == -1 || info.second[0][corres["released"]].as<bool>() == true) return -1;
		std::ostringstream q;
		q << "UPDATE " << tableName << " SET released = true WHERE trainID = \'" << trainID << "\';";
		int ret = c -> executeTrans(q.str());
		std::string saleDate = info.second[0][corres["saleDate"]].as<std::string>();
		int stationNum = info.second[0][corres["stationNum"]].as<int>();
		std::string seatNum = info.second[0][corres["seatNum"]].as<std::string>();
		if (ret == 0) {
			query -> init_ticket(trainID, saleDate, stationNum, seatNum);
			auto info = getTrainInfo(trainID);
			std::vector<std::string> stations = arrayParser<std::string>::parse(
				info.second[0][corres["stations"]].as<std::string>());
			for (int i = 0; i < stations.size(); i++) {
				std::ostringstream q;
				q << "INSERT INTO " << "city2trainIDtable" << " (city,trainID) "
				  << "VALUES (\'" << stations[i] << "\', \'" << trainID << "\');";
				c -> executeTrans(q.str());
			}
		}
		return ret;
	}
	
	int delete_train(std::string trainID) {
		auto info = getTrainInfo(trainID);
		if (info.first == -1 || info.second[0][corres["released"]].as<bool>() == true) return -1;
		std::ostringstream q;
		q << "DELETE FROM " << tableName << " WHERE trainID = \'" << trainID << "\';";
		return c -> executeTrans(q.str());
	}

	std::vector<std::string> findTrainId(std::string city) {
		std::ostringstream q;
		q << "SELECT * FROM " << "city2trainIDtable" << " WHERE city = \'" << city << "\';";
		auto qry = c -> executeNonTrans(q.str());
		std::vector<std::string> ret;
		for (pqxx::result::const_iterator it = qry.second.begin(); it != qry.second.end(); it++)
			ret.push_back(it[1].as<std::string>());
		return ret;
	}

	std::pair<std::string, std::string> findTime(std::string trainID, std::string date, std::string FROM, std::string TO) {
		auto info = getTrainInfo(trainID);
		std::vector<std::string> stations = arrayParser<std::string>::parse(info.second[0][corres["stations"]].as<std::string>());
		std::vector<int> travelTimes = arrayParser<int>::parse(info.second[0][corres["travelTimes"]].as<std::string>());
		std::vector<int> stopoverTimes = arrayParser<int>::parse(info.second[0][corres["stopoverTimes"]].as<std::string>());
		std::string startTime = info.second[0][corres["startTime"]].as<std::string>();
		int stationNum = info.second[0][corres["stationNum"]].as<int>();
		moment curTime(date, startTime);
		std::pair<std::string, std::string> ret;
		for (int i = 1; i < stationNum; i++) {
			if (stations[i - 1] == FROM) ret.first = curTime.toString();
			curTime += travelTimes[i];
			if (stations[i] == TO) ret.second = curTime.toString();
			curTime += stopoverTimes[i];
		}
	}
};
