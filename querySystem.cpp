#include "userSystem.h"
#include "trainSystem.h"
#include "ticketSystem.h"
#include "querySystem.h"

std::vector<std::string> querySystem::intersection(std::vector<std::string> a, std::vector<std::string> b) {
    std::vector<std::string> ret;
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(ret));
    return ret;
}

querySystem::querySystem(database *_c) : c(_c) {
    std::string sql = "CREATE TABLE IF NOT EXISTS ticketInfo(" \
        "trainID varchar(255) PRIMARY KEY," \
        "ticketNum integer[][] );";
    c -> executeTrans(sql);
}

void querySystem::init_system(ticketSystem *_ticketsys, trainSystem *_trainsys, userSystem *_usersys) {
    ticketsys = _ticketsys;
    trainsys = _trainsys;
    usersys = _usersys;
}

//saleDate eg. 06-01|08-17, success ensured
void querySystem::init_ticket (std::string trainID, std::string saleDate, int stationNum, std::string ticketNum) {
    auto d = arrayParser<std::string>::parse(saleDate);
    moment start(d[0], "00:00"), end(d[1], "23:59");
    std::string array, finalArray;
    for (int i = 0; i < stationNum; i++) array += ticketNum + (i + 1 < stationNum ? "," : "");
    array = "{ " + array + " }";
    for (int day = start.day; day <= end.day; day++) finalArray += array + (day + 1 <= end.day ? "," : "");
    finalArray = "{ " + finalArray + " }";
    std::ostringstream sql;
    sql << "INSERT INTO ticketInfo VALUES ( " << "\'" << trainID << "\', \'" << finalArray << "\');";
    c -> executeTrans(sql.str());
}

void querySystem::add_ticket(std::string trainId, std::string date, std::string str_num, std::string FROM, std::string TO) {
    int num = stoi(str_num);
    auto info = (trainsys -> getTrainInfo(trainId)).second[0];
    auto sale = arrayParser<std::string>::parse(info[trainSystem::corres["saleDate"]].as<std::string>());
    auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
    auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
    auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());
    int s = -1, t = -1;
    moment curMin(sale[0], info[trainSystem::corres["startTime"]].as<std::string>());
    for (int i = 0; i < stations.size(); i++) {
        if (stations[i] == FROM) s = i;
        if (stations[i] == TO) {
            t = i;
            break;
        }
        if (s == -1) curMin += travelTimes[i] + stopOverTimes[i];
    }
    s++, t++;
    auto ticket = getTicket(trainId, moment(date,"xx:xx").day - curMin.day, s, t);
    if (ticket.size() == 0) return;
    std::ostringstream sql;
    sql << "UPDATE ticketInfo SET ticketNum[" << moment(date,"xx:xx").day - curMin.day + 1 << ":" << moment(date,"xx:xx").day - curMin.day + 1
        << "][" << s << ":" << t << "]=\'{";
    for (int i = s; i <= t; i++) sql << ticket[i] + num << (i != t ? "," : "");
    sql << "}\';";
    c -> executeTrans(sql.str()); 
}

void querySystem::add_ticket(std::string trainId, std::string date, std::string station) {
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

// return value: (<price>, "queue") or (-1, "") or (<price>, "")
std::pair<int, std::string> querySystem::buy_ticket (std::string userName, std::string trainId, std::string date,
        std::string str_num, std::string FROM, std::string TO, std::string queue) {
    int num = stoi(str_num);
    if (!usersys->checkUser(userName)) return std::make_pair(-1, "");
    auto info = (trainsys -> getTrainInfo(trainId)).second[0];
    auto sale = arrayParser<std::string>::parse(info[trainSystem::corres["saleDate"]].as<std::string>());
    auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
    auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
    auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());
    auto prices = arrayParser<int>::parse(info[trainSystem::corres["prices"]].as<std::string>());

    int s = -1, t = -1, price = 0;
    moment curMin(sale[0], info[trainSystem::corres["startTime"]].as<std::string>());
    for (int i = 0; i < stations.size(); i++) {
        price += prices[i];
        if (stations[i] == FROM) s = i, price = 0;
        if (stations[i] == TO) {
            t = i;
            break;
        }
        if (s == -1) curMin += travelTimes[i] + stopOverTimes[i];
    }
    if (s == -1 || t == -1) return std::make_pair(-1, "");
    s++, t++;
    int rem = getMinTicket(trainId, moment(date,"xx:xx").day - curMin.day, s, t);
    std::cout << "[rem] " << rem << std::endl;
    if (rem >= num) {
        auto ticket = getTicket(trainId, moment(date,"xx:xx").day - curMin.day, s, t);
        std::ostringstream sql;
        sql << "UPDATE ticketInfo SET ticketNum[" << moment(date,"xx:xx").day - curMin.day + 1 << ":" << moment(date,"xx:xx").day - curMin.day + 1
            << "][" << s << ":" << t << "]=\'{";
        for (int i = s; i <= t; i++) sql << ticket[i] - num << (i != t ? "," : "");
        sql << "}\' WHERE trainID = \'" << trainId << "\';";
        c -> executeTrans(sql.str()); 
        return std::make_pair(price * num, "");
    }
    else return std::make_pair(-1, queue == "false" ? "queue" : "");
}

std::vector<int> querySystem::parseTicket(pqxx::result t) {
    std::vector<int> ticket;
    if (t.size() == 0) return ticket;

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

std::vector<int> querySystem::getTicket(std::string trainId, int day, int s, int t) {
    std::ostringstream sql;
    sql << "SELECT " << "ticketNum[" << day + 1 << " : " << day + 1 << "][:] FROM ticketInfo WHERE trainID = \'" << trainId << "\';";
    auto ticket = parseTicket(c -> executeNonTrans(sql.str()).second);
    return ticket;
}

int querySystem::getMinTicket(std::string trainId, int day, int s, int t) {
    std::ostringstream sql;
    sql << "SELECT " << "ticketNum[" << day + 1 << " : " << day + 1 << "][:] FROM ticketInfo WHERE trainID = \'" << trainId << "\';";
    auto ticket = parseTicket(c -> executeNonTrans(sql.str()).second);
    if (ticket.size() == 0) return -1;
    int ret = INT_MAX;
    for (int i = s - 1; i <= t - 1; i++) ret = std::min(ret, ticket[i]);
    // std::cout << "GET MIN TICKET: " << ret << std::endl;
    return ret;
}

//int is useless, just put all the output in string
std::pair<int, std::string> querySystem::query_ticket(std::string start, std::string end, std::string date, std::string mode, bool bestOnly) {
    auto trainIds = intersection(trainsys -> findTrainId(start), trainsys -> findTrainId(end));
    std::vector<std::pair<int, std::string>> allTrain;

    for (auto trainId: trainIds) {
        auto info = (trainsys -> getTrainInfo(trainId)).second[0];
        auto sale = arrayParser<std::string>::parse(info[trainSystem::corres["saleDate"]].as<std::string>());
        auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
        auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
        auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());
        auto prices = arrayParser<int>::parse(info[trainSystem::corres["prices"]].as<std::string>());

        moment curMin(sale[0], info[trainSystem::corres["startTime"]].as<std::string>());
        moment curMax(sale[1], info[trainSystem::corres["startTime"]].as<std::string>());
        moment *t1, *t2;

        int price = 0, t1s, t2s;
        for (int i = 0; i < stations.size(); i++) {
            curMin += travelTimes[i];
            curMax += travelTimes[i];
            price += prices[i];
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

            curMin += stopOverTimes[i];
            curMax += stopOverTimes[i];
        }
        int daysElapsed = moment(date, "xx:xx").day - curMin.day;			
        if (moment(date, "xx:xx").day >= curMin.day && moment(date, "xx:xx").day <= curMax.day) {
            //transform to requesting day
            t1->day += daysElapsed;
            t2->day += daysElapsed;
            t1s++, t2s++;
            int ticketNum = getMinTicket(trainId, daysElapsed, t1s, t2s);
            // std::cout << "MIN TICKET: " << ticketNum << std::endl;
            std::ostringstream tmp;
            tmp << trainId << " " << start << " " << t1->toString() << "->" << end << " " << t2->toString() << " " << price << " " << ticketNum << "\n";
            allTrain.push_back(std::make_pair(mode == "cost" ? price : curMin.toInt(), tmp.str()));
        }
    }
    sort(allTrain.begin(), allTrain.end());
    if (!bestOnly) {
        std::ostringstream ret;
        ret << allTrain.size() << "\n";
        for (auto t: allTrain) ret << t.second << "\n";
        return std::make_pair(0, ret.str());
    } else {
        return std::make_pair(0, allTrain[0].second);
    }
}

std::pair<std::vector<std::string>, std::vector<int>> querySystem::getAllStationFrom(std::vector<std::string> trains, std::string start) {
    std::vector<std::string> ret1;
    std::vector<int> ret2;
    for (auto trainId: trains) {
        auto info = (trainsys -> getTrainInfo(trainId)).second[0];
        auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
        auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
        auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());

        bool flg = false;
        int cost = 0;
        for (int i = 0; i < stations.size(); i++) {
            cost += travelTimes[i];
            if (flg) {
                ret1.push_back(stations[i]);
                ret2.push_back(cost);
            }
            if (stations[i] == start) flg = true;
            cost += stopOverTimes[i];
        }
    }
    return std::make_pair(ret1, ret2);
}

std::pair<std::vector<std::string>, std::vector<int>> querySystem::getAllStationUntil(std::vector<std::string> trains, std::string end) {
    std::vector<std::string> ret1;
    std::vector<int> ret2;
    for (auto trainId: trains) {
        auto info = (trainsys -> getTrainInfo(trainId)).second[0];
        auto stations = arrayParser<std::string>::parse(info[trainSystem::corres["stations"]].as<std::string>());
        auto travelTimes = arrayParser<int>::parse(info[trainSystem::corres["travelTimes"]].as<std::string>());
        auto stopOverTimes = arrayParser<int>::parse(info[trainSystem::corres["stopOverTimes"]].as<std::string>());

        bool flg = false;
        int cost = 0;
        for (int i = stations.size() - 1; i >= 0; i--) {
            if (flg) {
                cost += travelTimes[i];
                ret1.push_back(stations[i]);
                ret2.push_back(cost);
                cost += stopOverTimes[i];
            }
            if (stations[i] == end) flg = true;
        }
    }
    return std::make_pair(ret1, ret2);
}

//int is useless, just put all the output in string
std::pair<int, std::string> querySystem::query_transfer(std::string start, std::string end, std::string date, std::string mode) {
    auto s1 = getAllStationFrom(trainsys -> findTrainId(start), start);
    auto s2 = getAllStationUntil(trainsys -> findTrainId(end), end);
    auto common = intersection(s1.first, s2.first);
    int minCost = INT_MAX;
    std::string minStation;
    for (auto t: common) {
        int cur = s1.second[std::find(s1.first.begin(), s1.first.end(), t) - s1.first.begin()] + s2.second[std::find(s2.first.begin(), s2.first.end(), t) - s2.first.begin()];
        if (cur < minCost) {
            minCost = cur;
            minStation = t;
        }
    }
    auto ret1 = query_ticket(start, minStation, date, "time", true);
    auto ret2 = query_ticket(minStation, end, date, "time", true);
    return std::make_pair(0, ret1.second + "\n" + ret2.second);
}