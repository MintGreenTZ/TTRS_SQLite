#pragma once
#include <iostream>
#include <set>
#include <string>
#include <sstream>
#include <pqxx/pqxx>
#include "database.hpp"

class userSystem {
    friend class querySystem;

public:
    static std::map<std::string, int> corres;
    
private:
    database *c;
    std::string tableName;
    std::set<std::string> curUsers;
    bool init;

private:
    bool checkFirst();

    int addCurUser(std::string username);

    int delCurUser(std::string username);

    bool checkUser(std::string username);

private:
    std::pair<int, pqxx::result> getProfile(std::string username);

    int getPrivilege(std::string username);

    std::string toString(pqxx::result t);

public:
    userSystem(database *_c, std::string _tableName);

    void clear();
    
    int addUser(std::string cusername, std::string username, std::string password, std::string name, std::string mailAddr, std::string privilege);

    int login(std::string username, std::string password);

    int logout(std::string username);

    std::pair<int, std::string> query_profile(std::string cusername, std::string username);

    std::pair<int, std::string> modify_profile(std::string cusername, std::string username, 
                std::string password, std::string name, std::string mailAddr, std::string privilege);
};