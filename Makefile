.PHONY: all clean
	
all: andor-daemon client
	
clean:
			rm -rf andor-daemon client ./source/*.o
andor-daemon: source/andor-daemon.cpp source/andor-daemon.h
			g++ -o andor-daemon source/andor-daemon.cpp -lconfig -lcfitsio -landor
client: source/client.cpp
			g++ -o client source/client.cpp