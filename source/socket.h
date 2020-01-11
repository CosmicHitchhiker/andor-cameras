#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"
#include <string>
#include <cstring>
#include <csignal>
//#include <boost/algorithm/string.hpp>

#ifndef SOCKET_H
#define SOCKET_H

class Socket
{
  public:
	Socket(int port_number, Log* logFile, int mode = 0);
	~Socket();
	std::string getMessage();
	void sendMessage(char* message);
	void answer(const char* message);
	void turnOff();
	static void myError(int);
	int getMaxLen();
	int getDescriptor();

  private:
  	int portNo;	// Номер порта
  	int listener;  // Основной сокет для приёма данных
  	struct sockaddr_in addr;
  	int msg_sock;  // Создаваемый для приёма команды сокет
	Log* log;
	const int MESSAGE_LEN = 1024;
};

#endif // SOCKET_H