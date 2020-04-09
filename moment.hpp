#pragma once

#include <string>

class moment {
	friend class querySystem;

private:
	int month, day, hour, minute;

	std::string twoBit(int x) {
		std::string ret = std::to_string(x);
		if (ret.length() == 1)
			return "0" + ret;
		else
			return ret;
	}
public:
	static const int duration[13];
	static const int durationSum[13];
	static const std::string emptyMoment;

	moment(std::string date, std::string time) {
		month = atoi(date.substr(0, 2).c_str());
		day = atoi(date.substr(3, 2).c_str());
		hour = atoi(time.substr(0, 2).c_str());
		minute = atoi(time.substr(3, 2).c_str());
	}

	moment(int& _month, int& _day, int& _hour, int& _minute)
		:month(_month), day(_day), hour(_hour), minute(_minute) {}

	std::string toString() {
		return twoBit(month) + "-" + twoBit(day) + " " + twoBit(hour) + ":" + twoBit(minute);
	}

	int toInt() {
		return (durationSum[month] + day) * 1440 + hour * 60 + minute;
	}

	moment operator +(int dminute) {
		moment ret(month, day, hour, minute);

		ret.minute += dminute;

		ret.hour += ret.minute / 60;
		ret.minute = ret.minute % 60;

		ret.day += ret.minute / 24;
		ret.hour = ret.hour % 24;

		if (ret.day > duration[ret.month]) {
			ret.day -= duration[ret.month];
			ret.month++;
		}

		if (ret.month == 13)
			ret.month = 1;

		return ret;
	}

	void operator +=(int dminute) {
		*this = *this + dminute;
	}
};

const int moment::duration[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const int moment::durationSum[13] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

const std::string moment::emptyMoment = "xx-xx xx:xx";

