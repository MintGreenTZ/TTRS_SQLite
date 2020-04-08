#pragma once

#include <iostream>
#include <sstream>
#include <map>
#include <pqxx/pqxx>
#include "moment.hpp"
#include "trainSystem.hpp"
#include "ticketSystem.hpp"
#include "database.hpp"
#include <climits>
#include <string>
#include "arrayParser.hpp"

class querySystem {	
private:
	ticketSystem *ticketsys;
	trainSystem *trainsys;
	database *c;

private:
	std::vector<std::string> intersection(std::vector<std::string> a, std::vector<std::string> b) {
		std::vector<std::string> ret;
		std::sort(a.begin(), a.end());
		std::sort(b.begin(), b.end());
		std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(ret));
		return ret;
	}

public:
	querySystem(/*ticketSystem *_ticketsys, trainSystem *_trainsys, */ database *_c) : /*ticketsys(_ticketsys), trainsys(_trainsys),*/ c(_c) {
		std::string sql = "CREATE TABLE IF NOT EXISTS ticketInfo(" \
			"trainID varchar(255) PRIMARY KEY," \
			"ticketNum integer[][] );";
		c -> executeTrans(sql);
	}

public:
	//saleDate eg. 06-01|08-17, success ensured
	void init_ticket (std::string trainID, std::string saleDate, std::string stationNum, int ticketNum) {
		auto d = arrayParser<std::string>::parse(saleDate);
		moment start(d[0], "00:00"), end(d[1], "23:59");
		std::string array, finalArray;
		for (int i = 0; i < stoi(stationNum); i++) array += std::to_string(ticketNum) + (i + 1 < stoi(stationNum) ? "," : "");
		array = "{ " + array + " }";
		for (int day = start.day; day <= end.day; day++) finalArray += array + (day + 1 <= end.day ? "," : "");
		finalArray = "{ " + finalArray + " }";
		std::ostringstream sql;
		sql << "INSERT INTO ticketInfo VALUES ( " << "\'" << trainID << "\', \'" << finalArray << "\');";
		c -> executeTrans(sql.str());  
	}
	
	void add_ticket(std::string trainId, std::string date, std::string station) {
		auto info = (trainsys -> getTrainInfo(trainId)).second[0];
		auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
		auto sale = moment(arrayParser<std::string>::parse(info[trainSystem::corres["saleDate"]].as<std::string>())[0], "00:00");
		int elapsedDay = sale.day - moment(date, "00:00").day;
		for (int i = 0; i < stations.size(); i++) {
			if (stations[i] != station) continue;
			std::ostringstream sql;
			sql << "SELECT " << "ticketNum[" << elapsedDay + 1 << "][" << i + 1 << "] FROM ticketInfo WHERE trainID = \'" << trainId << "\';";
			int cur = (c -> executeNonTrans(sql.str())).second[0][0].as<int>();
			sql << "UPDATE ticketInfo SET ticketNum[" << elapsedDay + 1 << "][" << i + 1 << "]=\'" << cur + 1 << "\' WHERE trainID = \'" << trainId << "\';";
			c -> executeTrans(sql.str());  
			break;
		}
	}

	// return value: (-1, "queue") or (-1, "") or (<price>, "")
	std::pair<int, std::string> buy_ticket (std::string userName, std::string trainID, std::string date,
			std::string FROM, std::string TO, std::string queue = "false") {
		//TODO
	}

public:
	std::vector<int> parseTicket(pqxx::result t) {
		std::vector<int> ticket;
		auto parser = t[0][0].as_array();
		auto obj = parser.get_next();
		while (obj.first != pqxx::array_parser::juncture::done) {
			if (obj.first == pqxx::array_parser::juncture::string_value) {
				ticket.push_back(stoi(obj.second));
			}
			obj = parser.get_next();
		}
		return ticket;
	}

	int getMinTicket(std::string trainId, int day, int s, int t) {
		std::ostringstream sql;
		sql << "SELECT " << "ticketNum[" << day + 1 << " : " << day + 1 << "][:] FROM ticketInfo WHERE trainID = \'" << trainId << "\';";
		auto ticket = parseTicket(c -> executeNonTrans(sql.str()).second);
		int ret = INT_MAX;
		for (int i = s; i <= t; i++) ret = std::min(ret, ticket[i]);
		return ret;
	}

public:
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_ticket(std::string start, std::string end, std::string date, std::string mode) {
		auto trainIds = intersection(trainsys -> findTrainId(start), trainsys -> findTrainId(end));
		int cnt = 0;
		std::ostringstream tmp;

		for (auto trainId: trainIds) {
			auto info = (trainsys -> getTrainInfo(trainId)).second[0];
			auto sale = arrayParser<std::string>::parse(info[trainSystem::corres["saleDate"]].as<std::string>());
			auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
			auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
			auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());
			auto prices = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());

			moment curMin(sale[0], info[trainSystem::corres["startTime"]].as<std::string>());
			moment curMax(sale[0], info[trainSystem::corres["startTime"]].as<std::string>());
			moment *t1, *t2;

			int price = 0, t1s, t2s;
			for (int i = 0; i < stations.size(); i++) {
				price += i < 1 ? 0 : prices[i - 1];
				if (stations[i] == end) {
					t2 = &curMin;
					t2s = i;
					break;
				}	
				if (stations[i] == start) {
					t1 = &curMin;
					t1s = i;
					price = 0;
				}
				curMin += travelTimes[i] + stopOverTimes[i];
				curMax += travelTimes[i] + stopOverTimes[i];
			}
			int daysElapsed = moment(date, "xx:xx").day - curMin.day;			
			if (moment(date, "xx:xx").day >= curMin.day && moment(date, "xx:xx").day <= curMax.day) {
				//transform to requesting day
				t1->day += daysElapsed;
				t2->day += daysElapsed;
				cnt++;
				int ticketNum = getMinTicket(trainId, daysElapsed, t1s, t2s);
				tmp << trainId << " " << start << " " << t1->toString() << "->" << end << " " << t2->toString << " " << price << " " << ticketNum << "\n";
			}
		}
		std::ostringstream ret;
		ret << cnt << "\n" << tmp.str();
		return std::make_pair(0, ret.str());
	}
	
	//int is useless, just put all the output in string
	std::pair<int, std::string> query_transfer(std::string start, std::string end, std::string date, std::string mode) {
		//TODO
	}
};
