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

	moment() {
		/* EMPTY */
	}
	
	moment(std::string date, std::string time) {
		month = atoi(date.substr(0, 2).c_str());
		day = atoi(date.substr(3, 2).c_str());
		hour = atoi(time.substr(0, 2).c_str());
		minute = atoi(time.substr(3, 2).c_str());
	}

	moment(int& _month, int& _day, int& _hour, int& _minute)
		:month(_month), day(_day), hour(_hour), minute(_minute) {}

	std::string toString() {
		hour += minute / 60; minute %= 60;
		day += hour / 24; hour %= 24;
		while (day > duration[month]) day -= duration[month], month += 1;
		return twoBit(month) + "-" + twoBit(day) + " " + twoBit(hour) + ":" + twoBit(minute);
	}

	int toInt() {
		return (durationSum[month - 1] + (day - 1)) * 1440 + hour * 60 + minute;
	}

	int toDay() {
		//Used for calculating difference between two events in terms of day
		return durationSum[month - 1] + (day - 1);
	}

	moment operator +(int dminute) {
		moment ret(month, day, hour, minute);

		ret.minute += dminute;

		ret.hour += ret.minute / 60;
		ret.minute = ret.minute % 60;

		ret.day += ret.hour / 24;
		ret.hour = ret.hour % 24;

		if (ret.day > duration[ret.month]) {
			ret.day -= duration[ret.month];
			ret.month++;
		}

		if (ret.month == 13) ret.month = 1;

		return ret;
	}

	void operator +=(int dminute) {
		*this = *this + dminute;
	}
};

