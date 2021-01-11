#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <vector>
#include <cstdarg>

#ifndef LOG_H
#define LOG_H

class Log
{
  public:
	Log();
	Log(std::string fileName);
	~Log();
	void print(const char* message, ...);

  protected:
  	std::string format(const char *fmt, ...);
  	std::string format(const char *fmt, va_list args);

  private:
  	std::ofstream file;
  	std::string name;	
};

#endif // LOG_H