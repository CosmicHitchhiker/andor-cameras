#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>

int main()
{
    char message[1024] = {0}, buf[1024] = {0};
    int sock;
    struct sockaddr_in addr;
    int port_number;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock < 0)
    {
        std::cout << "Can't create socket\n";
        return 1;
    }
    std::cout << "Port port_number:";
    std::cin >> port_number;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_number); // или любой другой порт...
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::cout << "Can't connect\n";
        return 2;
    }
    std::cin >> port_number;
    fgets(message, 100, stdin);
    send(sock, message, sizeof(message), 0);
    // recv(sock, buf, 1024, 0);

    // std::cout << buf;
    close(sock);

    return 0;
}
