#pragma once

#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "arrayParser.hpp"
#include "moment.hpp"
#include "querySystem.h"

class querySystem;

class trainSystem {
public:
	static std::map<std::string, int> corres;
private:
	database *c, *city2trainID;
	std::string tableName;
	querySystem *query;
public:
	std::pair<int, pqxx::result> getTrainInfo(std::string trainID);
public:
	trainSystem(database *_c, std::string _tableName, querySystem *_query);

	int add_train(std::string trainID, std::string stationNum, std::string seatNum, std::string stations, 
				std::string prices, std::string startTime, std::string travelTimes, 
				std::string stopoverTimes, std::string saleDate, std::string type);

	std::pair<int, std::string> query_train(std::string trainID, std::string date);

	int release_train(std::string trainID);
	
	int delete_train(std::string trainID);

	std::vector<std::string> findTrainId(std::string city);

	std::pair<std::string, std::string> findTime(std::string trainID, std::string date, std::string FROM, std::string TO);
};
