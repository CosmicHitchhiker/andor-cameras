.PHONY: all clean

TARG = ../
TEST = ../tests/

all: client andor-daemon andor-daemon-test

client: client.o log.o socket.o
	g++ -std=c++11 client.o log.o socket.o -o $(TARG)client

#andor-daemon: andor-daemon-new.o log.o socket.o header_values.o camera.o
#	g++ andor-daemon-new.o log.o socket.o header_values.o camera.o -o andor-daemon

andor-daemon: andor-daemon-new.o log.o socket.o header_values.o camera.o
	g++ -std=c++11 andor-daemon-new.o log.o socket.o header_values.o camera.o -o $(TARG)andor-daemon -landor -lcfitsio -lconfig++

andor-daemon-test: andor-daemon-test.o log.o socket.o header_values.o camera.o virtual_camera.o
	g++ -std=c++11 $(TEST)andor-daemon-test.o log.o socket.o header_values.o camera.o $(TEST)virtual_camera.o -o $(TEST)andor-daemon-test -landor -lcfitsio -lconfig++

andor-daemon-test.o: $(TEST)andor-daemon-test.cpp
	g++ -std=c++11 -c $(TEST)andor-daemon-test.cpp -o $(TEST)andor-daemon-test.o

virtual_camera.o: $(TEST)virtual_camera.cpp $(TEST)virtual_camera.h
	g++ -std=c++11 -c $(TEST)virtual_camera.cpp -o $(TEST)virtual_camera.o

client.o: client.cpp
	g++ -std=c++11 -c client.cpp

andor-daemon-new.o: andor-daemon-new.cpp
	g++ -std=c++11 -c andor-daemon-new.cpp

log.o: log.cpp
	g++ -std=c++11 -c log.cpp

socket.o: socket.cpp socket.h
	g++ -std=c++11 -c socket.cpp

header_values.o: header_values.cpp header_values.h
	g++ -std=c++11 -c header_values.cpp

camera.o: camera.cpp camera.h
	g++ -std=c++11 -c camera.cpp

clean:
	rm -rf *.o $(TARG)andor-daemon $(TARG)client $(TEST)*.o $(TEST)andor-daemon-test
