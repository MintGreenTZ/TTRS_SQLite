#include "userSystem.h"
#include "querySystem.h"

bool userSystem::checkFirst() {
    std::string q = "select true from " + tableName + " limit 1;";
    auto ret = c -> executeNonTrans(q);
    return ret.second.size() == 0;
}

void userSystem::addUser(std::string username) {
    curUsers.insert(username);
}

void userSystem::delUser(std::string username) {
    curUsers.erase(username);
}

bool userSystem::checkUser(std::string username) {
    return curUsers.find(username) != curUsers.end();
}

std::pair<int, pqxx::result> userSystem::getProfile(std::string username) {
    std::ostringstream q; 
    q << "SELECT * FROM " << tableName << " WHERE username = \'" << username << "\';";
    auto ret = c -> executeNonTrans(q.str());
    auto suc = ret.first;
    auto content = ret.second;
    if (suc == -1 || content.size() != 1) return std::make_pair(-1, content);
    else return std::make_pair(0, content);
}

int userSystem::getPrivilege(std::string username) {
    auto t = getProfile(username);
    return t.first == -1 ? -1 : t.second[0][corres["privilege"]].as<int>();
}

std::string userSystem::toString(pqxx::result t) {
    std::ostringstream q; 
    q << t[0][corres["username"]].as<std::string>() << " " << t[0][corres["name"]].as<std::string>() << " "
        << t[0][corres["mailAddr"]].as<std::string>() << " " << t[0][corres["privilege"]].as<int>();
    return q.str();
}

userSystem::userSystem(database *_c, std::string _tableName) : c(_c), tableName(_tableName) {
    std::string sql = "CREATE TABLE IF NOT EXISTS usertable(" \
        "username varchar(255) PRIMARY KEY," \
        "password varchar(255)," \
        "name varchar(255)," \
        "mailAddr varchar(255)," \
        "privilege int ); ";
    c -> executeTrans(sql);
    init = checkFirst();        
}

int userSystem::addUser(std::string cusername, std::string username, std::string password, std::string name, std::string mailAddr, std::string privilege) {
    if (!init && getPrivilege(cusername) < std::stoi(privilege)) return -1;
    std::ostringstream q; 
    q << "INSERT INTO " << tableName << " (username,password,name,mailAddr,privilege) "
        << "VALUES (\'" << username << "\', \'" << password << "\', \'" << name << "\', \'" << mailAddr << "\', " << privilege
        << ");";
    init = false;
    return c -> executeTrans(q.str());
}

int userSystem::login(std::string username, std::string password) {
    auto t = getProfile(username);
    if (t.first == -1) return -1;

    auto content = t.second[0];
    auto ans = content[corres["password"]].as<std::string>();
    if (ans == password) {
        addUser(username);
        return 0;
    }
    return -1;
}

int userSystem::logout(std::string username) {
    if (checkUser(username)) {
        delUser(username);
        return 0;
    }
    return -1;
}

std::pair<int, std::string> userSystem::query_profile(std::string cusername, std::string username) {
    if (!checkUser(cusername)) return std::make_pair(-1, "");
    if (getPrivilege(cusername) < getPrivilege(username)) return std::make_pair(-1, "");

    auto t = getProfile(username);
    if (t.first == -1) return std::make_pair(-1, "");
    return std::make_pair(0, toString(t.second));
}

std::pair<int, std::string> userSystem::modify_profile(std::string cusername, std::string username, 
            std::string password, std::string name, std::string mailAddr, std::string privilege) {
    if (!checkUser(cusername)) return std::make_pair(-1, "");
    if (getPrivilege(cusername) < getPrivilege(username)) return std::make_pair(-1, "");

    std::vector<std::string> v;
    if (password != "") v.push_back("password = \'" + password + "\'");
    if (name != "") v.push_back("name = \'" + name + "\'");
    if (mailAddr != "") v.push_back("mailAddr = \'" + mailAddr + "\'");
    if (privilege != "") v.push_back("privilege = " + privilege);
    std::string s;
    for (auto it = v.begin(); it != v.end(); it++) s += (it != v.begin() ? ", " : "") + *it;

    if (s != "") {
        std::ostringstream q; 
        q << "UPDATE " << tableName << " SET " << s << " WHERE username = \'" << username << "\';";
        c -> executeTrans(q.str());
    }
    return query_profile(cusername, username);
}
