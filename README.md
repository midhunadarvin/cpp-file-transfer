# File transfer using socket programming in CPP

Server.cpp : Defines the entry point for the server console application.

    Build:
    POSIX   => g++ -o Server.exe Server.cpp Payload.cpp -lpthread
    WINDOWS => cl /DWINDOWS_OS MServer.cpp ws2_32.lib

    Run:
    POSIX   => ./Mserver.exe <port>
    WINDOWS => Mserver <port>
    
    
Client.cpp : Defines the entry point for the client console application.

    Build:
    POSIX   => g++ -o Client.exe Client.cpp Payload.cpp
    WINDOWS => cl /DWINDOWS_OS Client.cpp ws2_32.lib
    
    Run:
    POSIX   => ./Client.exe <file-name> <hostname> <port>
    WINDOWS => Client <file-name> <hostname> <port>
    
    Eg. ./Client.exe sample.txt 127.0.0.1 8000

Based on code referrence from [@praseedpai](https://github.com/praseedpai)
