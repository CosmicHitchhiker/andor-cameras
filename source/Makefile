.PHONY: all clean

all: client andor-daemon-test

client: client.o log.o socket.o
	g++ -std=c++11 client.o log.o socket.o -o client

#andor-daemon: andor-daemon-new.o log.o socket.o header_values.o camera.o
#	g++ andor-daemon-new.o log.o socket.o header_values.o camera.o -o andor-daemon

andor-daemon-test: andor-daemon-new.o log.o socket.o header_values.o camera.o virtual_camera.o
	g++ -std=c++11 andor-daemon-new.o log.o socket.o header_values.o camera.o virtual_camera.o -o andor-daemon -landor -lcfitsio

client.o: client.cpp
	g++ -std=c++11 -c client.cpp

andor-daemon-new.o: andor-daemon-new.cpp
	g++ -std=c++11 -c andor-daemon-new.cpp

log.o: log.cpp
	g++ -std=c++11 -c log.cpp

socket.o: socket.cpp
	g++ -std=c++11 -c socket.cpp

header_values.o:
	g++ -std=c++11 -c header_values.cpp

camera.o:
	g++ -std=c++11 -c camera.cpp

virtual_camera.o:
	g++ -std=c++11 -c virtual_camera.cpp

clean:
	rm -rf *.o andor-daemon-test
