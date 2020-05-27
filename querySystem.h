#pragma once

#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include <algorithm>
#include <climits>
#include <string>

#include "moment.hpp"
#include "arrayParser.hpp"

#include "database.hpp"

class userSystem;
class ticketSystem;
class trainSystem;

class querySystem {	
private:
	userSystem *usersys;
	ticketSystem *ticketsys;
	trainSystem *trainsys;
	database *c;

private:
	std::vector<std::string> intersection(std::vector<std::string> a, std::vector<std::string> b);

public:
	querySystem(database *_c);

	void init_system(ticketSystem *_ticketsys, trainSystem *_trainsys, userSystem *_usersys);

public:
	// saleDate eg. 06-01|08-17, success ensured
	void init_ticket (std::string trainID, std::string saleDate, int stationNum, std::string ticketNum);
	
	void add_ticket(std::string trainId, std::string date, std::string str_num, std::string FROM, std::string TO);

	// return ticket_num of <trainID> on <date>, used by query_train
	std::vector<int> query_ticket(std::string trainID, std::string date);

	// return value: (<price>, "queue") or (-1, "") or (<price>, "")
	std::pair<int, std::string> buy_ticket (std::string userName, std::string trainId, std::string date,
			std::string str_num, std::string FROM, std::string TO, std::string queue = "false", bool isBuy = true);

private:
	std::vector<int> parseTicket(pqxx::result t);

	std::vector<int> getTicket(std::string trainId, int day);
    
	int getMinTicket(std::string trainId, int day, int s, int t);

public:
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_ticket(std::string start, std::string end, std::string date, std::string mode = "time", bool bestOnly = false);
	
private:
	std::pair<std::vector<std::string>, std::vector<int>> getAllStationFrom(std::vector<std::string> trains, std::string start);

	std::pair<std::vector<std::string>, std::vector<int>> getAllStationUntil(std::vector<std::string> trains, std::string end);

public:
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_transfer(std::string start, std::string end, std::string date, std::string mode);
};
