#pragma once
#include <iostream>
#include <pqxx/pqxx>
#include <string>

class database {
private:
    pqxx::connection *conn;

public:
    database() {
        conn = new pqxx::connection("dbname = ttrs user = gabriel password = tangze hostaddr = 127.0.0.1 port = 5432");
        if (conn -> is_open()) {
            // std::cout << "Opened database successfully: " << conn -> dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
		}
    }

    int executeTrans(std::string exec) {
        // std::cout << "EXECUTING: " << exec << std::endl;
        try {
            pqxx::work W(*conn);
            W.exec(exec);
            W.commit();
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return -1;
        }
        return 0;
    }

    std::pair<int, pqxx::result> executeNonTrans(std::string exec) {
        // std::cout << "EXECUTING: " << exec << std::endl;
        pqxx::result R;
        try{
            pqxx::nontransaction N(*conn);
            R = pqxx::result(N.exec(exec));
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return std::make_pair(-1, R);
        }
        return std::make_pair(0, R);
    }

    ~database() {
        // conn -> disconnect();
    }
};
