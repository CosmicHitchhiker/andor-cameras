#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "log.h"
#include <string>
#include <cstring>
#include <csignal>
#include <ctime>
//#include <boost/algorithm/string.hpp>

#ifndef SOCKET_H
#define SOCKET_H

class Socket
{
  public:
	Socket(int port_number, Log* logFile, int mode = 0); // mode == 0 - сервер, mode == 1 - клиент
	~Socket();
	bool getMessage(std::string* messageToWrite);
	void sendMessage(char* message);
	void answer(const char* message);
	void turnOff();
	static void myError(int);
	int getMaxLen();
	int getDescriptor();
	bool acceptConnection();
	bool checkClient();

  private:
  	int portNo;	// Номер порта
  	int listener;  // Основной сокет для приёма данных
  	struct sockaddr_in addr;
  	int msg_sock;  // Создаваемый для приёма команды сокет
	Log* log;
	const int MESSAGE_LEN = 1024;
	// Если в течение 3 минут не будет сообщения - считаем, что клиент отсоеденился
	const double connectionTimeout = 3*60; 
	time_t timeLastConnection;
};

#endif // SOCKET_H