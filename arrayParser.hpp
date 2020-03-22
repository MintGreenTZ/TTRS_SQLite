#pragma once
#include <utility>
#include <string>
#include <vector>
#include <assert.h>
#include <type_traits>

template <typename T>
class arrayParser {
public:
	static std::string getNext(std::string &str) {
		std::string ret = str;
		auto p = str.find("|");
		if (p != std::string::npos) ret = str.substr(0, p), str = str.substr(p + 1, str.length() - 1 - p);
		else str = "";
		return ret;
	}

	static std::vector<T> parse(std::string str) {
		constexpr bool transToInt = std::is_same<T, int>::value;
		constexpr bool transToString = std::is_same<T, std::string>::value;
		static_assert(transToInt || transToString);

		while (str.length() >= 1 && (str[str.length() - 1] == '\r' || str[str.length() - 1] == '\n' || str[str.length() - 1] == ' ')) str = str.substr(0, str.length() - 1);

		std::vector<T> p;

		while (str != "") {
			if constexpr (transToInt)
				p.push_back(atoi(getNext(str).c_str()));
			else
				p.push_back(getNext(str));
		}

		return p;
	}
};
