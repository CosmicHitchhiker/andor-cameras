#include "log.h"

using namespace std;

Log::Log() {
	name = "logfile.log";
	file.open(name, ios::app);
	time_t curr_time = time(NULL);
	file << "\n\n\nLog beginning " << ctime(&curr_time) << endl;
}

Log::Log(std::string fileName) {
	name = fileName;
	file.open(name, ios::app);
}

Log::~Log(){
	time_t cur_time = time(NULL);
    file << format("\nLog ending %s\n", ctime(&cur_time));
	file.close();
}

void Log::print(const char* message, ...) {
    // Print string and time in log file
	time_t cur_time;
	struct tm *curr_time;
	const int bufferSize = 20;
	char* buffer = new char[bufferSize];
	va_list arglist;

	time(&cur_time);
	curr_time = localtime(&cur_time);
	strftime(buffer,bufferSize,"%T %h %d ",curr_time);
	file << buffer << ": ";
	delete []buffer;

	va_start(arglist, message);
	string str = format(message, arglist);
	va_end(arglist);
	file << str << endl;
}

string Log::format(const char *fmt, va_list args) {
    std::vector<char> v(1024);
    while (true)
    {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(v.data(), v.size(), fmt, args2);
        if ((res >= 0) && (res < static_cast<int>(v.size())))
        {
            va_end(args2);
            return std::string(v.data());
        }
        size_t size;
        if (res < 0)
            size = v.size() * 2;
        else
            size = static_cast<size_t>(res) + 1;
        v.clear();
        v.resize(size);
        va_end(args2);
    }
}

string Log::format(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vector<char> v(1024);
    while (true)
    {
        va_list args2;
        va_copy(args2, args);
        int res = vsnprintf(v.data(), v.size(), fmt, args2);
        if ((res >= 0) && (res < static_cast<int>(v.size())))
        {
            va_end(args);
            va_end(args2);
            return std::string(v.data());
        }
        size_t size;
        if (res < 0)
            size = v.size() * 2;
        else
            size = static_cast<size_t>(res) + 1;
        v.clear();
        v.resize(size);
        va_end(args2);
    }
}