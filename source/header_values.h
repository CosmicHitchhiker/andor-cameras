#include <vector>
#include <string>
// #include <variant>
#include "fitsio.h"
#include <boost/algorithm/string.hpp>

#ifndef HEADER_VALUES_H
#define HEADER_VALUES_H

class HeaderValues
{
  public:
	HeaderValues();
	~HeaderValues();
	void addKey(std::string key, char type = 's', std::string value = "", std::string comment = "");
	std::string parseString(std::string str);
	void printAll();
	void update(std::string filename);
  void checkType(char type, std::string value);

  private:
  	int n; // Количество ключей для добавления
  	std::vector<std::string> keys;
  	std::vector<char> types;
  	// std::vector<std::variant<int, double, std::string, float>> values;
  	std::vector<std::string> values;
  	std::vector<std::string> comments;
};

#endif // HEADER_VALUES_H