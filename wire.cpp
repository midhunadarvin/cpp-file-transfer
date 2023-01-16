/**
 * wire.cpp : Defines the entry point for the client console application.
 * 
 * Build:
 * POSIX   => g++ -o wire.exe wire.cpp Payload.cpp
 * WINDOWS => cl /DWINDOWS_OS wire.cpp ws2_32.lib
 * 
 * Run:
 * POSIX   => ./wire.exe <file-name> <hostname> <port>
 * WINDOWS => wire <file-name> <hostname> <port>
 * 
 * Eg. ./wire.exe sample.txt 127.0.0.1 8000
*/
#ifdef WINDOWS_OS
  #include <windows.h>
#else
  #include <unistd.h>       // defines miscellaneous symbolic constants and types, and declares miscellaneous functions.
  #include <sys/socket.h>   // contains sockets definitions
  #include <netinet/in.h>   // contains definitions for the internet protocol family.
  #include <arpa/inet.h>    // contains definitions for internet operations
  #include <netdb.h>        // contains definitions for network database operations.
  #include <pthread.h>      // contains function declarations and mappings for threading interfaces and defines a number of constants used by those functions.
#endif
#include <string.h>         // contains helper functions for string operations
#include <stdio.h>          // contains declarations and functions to help with input output operations
#include <iostream>         // contains declarations and functions to help with input output operations
#include <fstream>          // predefines a set or operations for handling files related to input and output.

#include "payload.h"

using namespace std;

// Helper Function To Start Socket - Initialize Socket Engine ..No code inside the Posix
bool StartSocket();

#ifdef WINDOWS_OS
    void Cleanup() {WSACleanup();}

    bool StartSocket()
    {
        WORD Ver;
        WSADATA wsd;
        Ver = MAKEWORD(2, 2);
        if (WSAStartup(Ver, &wsd) == SOCKET_ERROR)
        {
            WSACleanup();
            return false;
        }

        return true;
    }

    int SocketGetLastError()
    {
        return  WSAGetLastError();
    }

    int CloseSocket( SOCKET s ) {
        closesocket(s);
        return 0;
    }
#else
    #define SOCKET_ERROR (-1)
    #define SOCKET int
    void Cleanup() {} 
    bool StartSocket(){ return true; }
    int SocketGetLastError(){ return  0xFFFF; }
    int CloseSocket( int s ) { shutdown (s, 2); return 0; }
#endif


class CClientSocket
{
    char m_ServerName[255];
    int m_PortNumber;
    struct sockaddr_in m_Server;
    struct hostent *m_HostPointer;
    unsigned int m_addr;
    SOCKET m_ConnectSock;

    public:
        CClientSocket(char *ServerName, int PortNumber) {
            strcpy(m_ServerName, ServerName);
            m_PortNumber = PortNumber;
        }

        SOCKET GetSocket() { return m_ConnectSock; }

        bool Resolve() {
            if (isalpha(m_ServerName[0])) {
                m_HostPointer = gethostbyname(m_ServerName);
            } else {   
                /* Convert nnn.nnn address to a usable one */
                m_addr = inet_addr(m_ServerName);
                m_HostPointer = gethostbyaddr((char *)&m_addr, 4, AF_INET);
            }
            if (m_HostPointer == NULL)
            {
                return false;
            }

            memset(&m_Server, 0, sizeof(m_Server));

            memcpy(&(m_Server.sin_addr), m_HostPointer->h_addr, m_HostPointer->h_length);
            m_Server.sin_family = m_HostPointer->h_addrtype;
            m_Server.sin_port = htons(m_PortNumber);
            return true;
        }

        bool Connect()
        {
            m_ConnectSock = socket(AF_INET, SOCK_STREAM, 0);
            if (m_ConnectSock < 0)
            {
                return false;
            }

            if (connect(m_ConnectSock,
                        (struct sockaddr *)&m_Server,
                        sizeof(m_Server)) == SOCKET_ERROR)
            {
                return false;
            }
            return true;
        }

        bool Send(void *buffer, int len)
        {
            int RetVal = send(m_ConnectSock, (const char *)buffer, len, 0);
            if (RetVal == SOCKET_ERROR)
            {
                return false;
            }

            return true;
        }

        bool Receive(void *buffer, int *len)
        {
            cout << "......................Received The Function.... " << endl;
            int RetVal = recv(m_ConnectSock, (char *)buffer, *len, 0);
            cout << "......................Received The Function.... " << RetVal << endl;

            if (RetVal == 0 || RetVal == -1)
            {
                printf("Error at socket(): %ld\n", SocketGetLastError());
                return false;
            }

            *len = RetVal;
            return true;
        }

        int Close()
        {
            CloseSocket(m_ConnectSock);
            return 0;
        }
};

bool SendEOF(SOCKET s) {
    T_FILE_EOF eof = MakeEof();
    int bytes_send = send(s, (char *) &eof, sizeof(eof), 0);
    return bytes_send > 0 ;
}

bool ReadSocketBuffer(SOCKET s, char *bfr, int size) {
    int RetVal = recv(s ,bfr,size,0);
	 if ( RetVal == 0 || RetVal == -1) 
		return false;
    return true;
}

bool WriteSocketBuffer(SOCKET s, char *bfr, int size) {
    int RetVal = send(s ,bfr, size, 0);
	 if ( RetVal == 0 || RetVal == -1) 
		 return false;
    return true;
}

bool SendAcknowledgement(SOCKET s)
{
    T_ACK ack = MakeAck();
    int bytes_send = send(s, (char *)&ack, sizeof(ack), 0);
    return bytes_send > 0;
}

int main(int argc, char **argv )
{
    if (argc != 4) {
        printf("Usage: %s <file-name> <server-name> <port> \n\n", argv[0]);
        return -1;
    }

    if ( !StartSocket() ){
      	fprintf(stdout,"Failed To Initialize Socket Library\n"); 
        return -1;
    }

    // Get params from arguments
    char * file_name = argv[1];
    char * server_name = argv[2];
    int port = atoi(argv[3]);

    int size_file = ComputeFileSize(file_name);
    printf("File name: %s\tFile size: %d\n", file_name, size_file);

    // Initialize client socket class
    CClientSocket c_sock(server_name, port);

    // Resolve server and connect
    if (!c_sock.Resolve() ) {
        printf("Failed to Resolve Client Socket ............\n");
        return 1;
    }
        
    if (!c_sock.Connect())  {
        printf("Failed to Connect to the Server ............\n");
        return 1;
    }

    /* Socket connection has been created. Initiate protocol communation */

    /* Create Handshake packet and Send to socket */
    T_HAND_SHAKE send_packet = MakeHandShake();
    c_sock.Send(&send_packet, sizeof(send_packet));

    /*  Wait for Acknowledgment packet ( sleep 5s )*/
    #ifdef WINDOWS_OS
        Sleep(5);
    #else
        sleep(5);
    #endif

    /* Receive data from socket ( acknowledgment ) */
    char buffer[32];
    int num_read = 32;

    if ( !c_sock.Receive(buffer, &num_read) ) {
        cout << "Could not Receive Packet " << endl;
        return 1;
    }
    
    /* Copy the char buffer received from socket into a T_ACK struct */
    T_ACK ack;
    memcpy(&ack, buffer, sizeof(T_ACK));

    // Verify if it is acknowledgment packet
    if ( ack.packet_type != 2 ) {
        cout << "Acknowledged Packet is not correct...........\n" << endl;
        return 1;
    }

    // Received acknowledment and ready to transfer file
    cout <<endl << "Ready to Transfer File ...................\n" << endl;

    // Create file meta data struct and send to socket 
    T_FILE_META  file_meta_data = MakeFileMeta(file_name, size_file);
    c_sock.Send(&file_meta_data,sizeof(file_meta_data));

    // Receive acknowledgment from server
    if ( !c_sock.Receive(buffer, &num_read) ) {
        cout << "Could not Receive Packet " << endl;
        return 1;
    }
        
    memcpy(&ack, buffer, sizeof(T_ACK));
    if ( ack.packet_type != 2 ) {
        cout << "Acknowledged Packet is not correct......" << endl;
        return 1;
    }
    

    /* Send file in chunks of size */
    cout << "Now, we will iterate to send the content of the file......" << endl;

    // This stores the number of bytes read from the file
    int num = 0;

    // We will be sending chunk sizes of 5000
    int chunk_size = sizeof(T_FILE_CHUNK) + 5000;
    int packet_sequence = 1;
   
    // open file in read binary (rb) mode
    FILE * fp = fopen(file_name, "rb");

    if  (fp == nullptr) {
        cout << "Failed to Open File = " << file_name << endl;
        return 1;
    }

    
    cout << "Opened File.............." << file_name << endl;
    char  read_buffer[4096];

    /**
     * Here we are reading from the file in byte sizes of 4096. From each read, we are creating 
     * the file chunk packet to be send. Next we send the packet through the socket connection. 
    */
    while ( (num = fread(read_buffer, 1, 4096, fp)) == 4096 ) {

        cout << "Sequence      ............... " << packet_sequence << endl;
        cout << "Sending Bytes ............... " << num << endl;

        // Create file chunk packet
        T_FILE_CHUNK * chunk = MakeBufferPack(read_buffer, 4096, packet_sequence++);

        // Send file chunk packet
        if ( !c_sock.Send(chunk, chunk_size) ) {
            cout << "Send Failure " << endl;
            break;
        }

        // free up the chunk memory
        free(chunk);
    }
    cout << "Residue = " << num << endl; // Remaining bytes that was read when it came out of the above loop
    cout << "Coming out of the Send Loop " << "Send Residue" << endl;

    /**
     * If there are any residue bytes remaining, 
     * we will send it as a file chunk
    */
    if ( num > 0 ) {
        T_FILE_CHUNK * chunk = MakeBufferPack( read_buffer, num, packet_sequence++ );
        if ( !c_sock.Send(chunk, chunk_size) ) {
            cout << "Send Failure " << endl;
            return -1;
        }
    }
    
    // Close file pointer as we are done reading from the file.
    fclose(fp);
    cout << "Finished Sending File, About to send EOF " << endl;

    // Send EOF packet, so the server gets confirmation that all packets have been sent.
    SendEOF(c_sock.GetSocket());
    cout << "Finished Sending EOF " << endl ;

    // Close socket connection and clean up.
    c_sock.Close(); 
    Cleanup();
}
