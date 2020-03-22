class ticketSystem {
private:
public:
	ticketSystem(database *_c, std::string _tableName) : c(_c), tableName(_tableName) {
		std::string sql = "CREATE TABLE IF NOT EXISTS tickettable(" \
			"trainID varchar(255) PRIMARY KEY," \
			"date varchar(255)," \
			"ticketNum varchar(4096);";
		c->executeTrans(sql);
	}
};