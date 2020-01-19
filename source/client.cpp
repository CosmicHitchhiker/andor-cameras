#include <iostream>
#include <stdio.h>
#include "socket.h"
#include "log.h"

using namespace std;

int main()
{
    int port_number;
    Log log("client.log");


    cout << "Port number: ";
    cin >> port_number;
    cin.ignore(100, '\n');
    

    while (true){
        Socket sock(port_number, &log, 1);
        int msg_len = sock.getMaxLen();
        char* message_c = new char[msg_len];
        fgets(message_c, msg_len, stdin);
        if (strcmp(message_c,"quit\n") == 0) break;
        sock.sendMessage(message_c);
        sock.turnOff();
    }
    return 0;
}
