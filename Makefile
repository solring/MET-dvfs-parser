CC = g++
CFLAGS = -Wc++11-extensions

parseDFSSwitch: parseDFSswt.o Record.o
	$(CC) $(CFLAGS) -o parseDFSswt parseDFSswt.o Record.o

parseDFSswt.o: parseDFSswt.cpp Record.h
	$(CC) $(CFLAGS) -c parseDFSswt.cpp

Record.o: Record.cpp Record.h
	$(CC) -c Record.cpp

clean:
	rm -rf *.o parseDFSswt
	
