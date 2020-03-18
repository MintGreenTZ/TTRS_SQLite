#pragma once
#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <pqxx/pqxx>

class userSystem {
private:
    std::map<std::string, int> corres =  { {"username", 0}, {"password", 1}, {"name", 2}, {"mailAddr", 3}, {"privilege", 4} };

    database *c;
    std::string tableName;
    std::set<std::string> curUsers;
    bool init;

private:
    bool checkFirst() {
        std::string q = "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE table_name = \'" + tableName + "\'";
        auto ret = c -> executeNonTrans(q);
        return ret.second.size() == 0;
    }

    void addUser(std::string username) {
        curUsers.insert(username);
    }

    void delUser(std::string username) {
        curUsers.erase(username);
    }

    bool checkUser(std::string username) {
        return curUsers.find(username) != curUsers.end();
    }

public:
    userSystem(database *_c, std::string _tableName) : c(_c), tableName(_tableName) {
        init = checkFirst();        
        std::string sql = "CREATE TABLE IF NOT EXISTS usertable(" \
            "username varchar(255) PRIMARY KEY," \
            "password varchar(255)," \
            "name varchar(255)," \
            "mailAddr varchar(255)," \
            "privilege int ); ";
        c -> executeTrans(sql);
    }

    int addUser(std::string username, std::string password, std::string name, std::string mailAddr, std::string priority) {
        //TODO: 当前用户权限
        std::ostringstream q; 
        q << "INSERT INTO " << tableName << " (username,password,name,mailAddr,privilege) "
            << "VALUES (\'" << username << "\', \'" << password << "\', \'" << name << "\', \'" << mailAddr << "\', " << priority
            << ");" ;
        return c -> executeTrans(q.str());
    }

    int login(std::string username, std::string password) {
        std::ostringstream q; 
        q << "SELECT * FROM " << tableName << " WHERE username = \'" << username << "\';";
        auto ret = c -> executeNonTrans(q.str());
        auto suc = ret.first;
        if (suc == -1) return -1;
        auto content = ret.second;
        if (content.size() != 1) return -1;

        auto ans = content[0][corres["password"]].as<std::string>();
        if (ans == password) {
            addUser(username);
            return 0;
        }
        return -1;
    }

    int logout(std::string username) {
        if (checkUser(username)) {
            delUser(username);
            return 0;
        }
        return -1;
    }
};