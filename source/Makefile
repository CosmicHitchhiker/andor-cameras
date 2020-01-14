.PHONY: all clean

all: client andor-daemon-test

client: client.o log.o socket.o
	g++ client.o log.o socket.o -o client

#andor-daemon: andor-daemon-new.o log.o socket.o header_values.o camera.o
#	g++ andor-daemon-new.o log.o socket.o header_values.o camera.o -o andor-daemon

andor-daemon-test: andor-daemon-new.o log.o socket.o header_values.o camera.o virtual_camera.o
	g++ andor-daemon-new.o log.o socket.o header_values.o camera.o virtual_camera.o -o andor-daemon -landor -lcfitsio

client.o: client.cpp
	g++ -c client.cpp

andor-daemon-new.o: andor-daemon-new.cpp
	g++ -c andor-daemon-new.cpp

log.o: log.cpp
	g++ -c log.cpp

socket.o: socket.cpp
	g++ -c socket.cpp

header_values.o:
	g++ -c header_values.cpp

camera.o:
	g++ -c camera.cpp

virtual_camera.o:
	g++ -c virtual_camera.cpp

clean:
	rm -rf *.o andor-daemon-test