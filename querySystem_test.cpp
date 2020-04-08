#include <iostream>
#include "querySystem.hpp"
#include "database.hpp"

int main() {
    database *db = new database();
    querySystem *sys = new querySystem(db);
    sys -> init_ticket("AAA", "06-01|06-15", "100", 100);
    sys -> init_ticket("BBB", "06-01|06-15", "100", 50);
    std::cout << sys -> getMinTicket("BBB", 14, 5, 10) << std::endl;
    return 0;
}