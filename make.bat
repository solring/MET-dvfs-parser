g++ -c Record.cpp
g++ -c parseDFSswt.cpp
g++ -static-libstdc++ -static-libgcc parseDFSswt.o Record.o -o parseDFSswt

g++ -o getStageData getStageData.cpp