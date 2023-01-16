/**
 * Server.cpp : Defines the entry point for the server console application.
 * 
 * Build:
 * POSIX   => g++ -o Server.exe Server.cpp Payload.cpp -lpthread
 * WINDOWS => cl /DWINDOWS_OS MServer.cpp ws2_32.lib
 * 
 * Run:
 * POSIX   => ./Mserver.exe <port>
 * WINDOWS => Mserver <port>
*/
#ifdef WINDOWS_OS
	#include <windows.h>
#else
  #include <sys/socket.h>   // contains sockets definitions
  #include <netinet/in.h>   // contains definitions for the internet protocol family.
  #include <arpa/inet.h>    // contains definitions for internet operations
  #include <netdb.h>        // contains definitions for network database operations.
  #include <pthread.h>      // contains function declarations and mappings for threading interfaces and defines a number of constants used by those functions.
#endif
#include <stdio.h>          // contains declarations and functions to help with input output operations ( fwrite )
#include <string.h>         // contains helper functions for string operations
#include <stdlib.h>         // defines four variable types, several macros, and various functions for performing general functions.
#include <ctype.h>          // declares several functions that are useful for testing and mapping characters.
#include <iostream>         // contains declarations and functions to help with input output operations

#include "payload.h"

using namespace std;

// Helper Function To Start Socket - Initialize Socket Engine ..No code inside the Posix
bool StartSocket();

#ifdef WINDOWS_OS
  typedef struct
  {
    SOCKET Sh; // Socket Handle which represents a Client
  } CLIENT_DATA;
  //---------------- Socket Descriptor for Windows
  //---------------- Listener Socket => Accepts Connection
  //---------------- Incoming Socket is Socket Per Client
  SOCKET ListenerSocket;
  SOCKET InComingSocket;

  //----------------- Thread Entry Points for Listener and Thread Per Client
  DWORD WINAPI ListenThreadProc(LPVOID lpParameter);
  DWORD WINAPI ClientThreadProc(LPVOID lpParam);

  //--------------- Call WSACleanUP for resource de-allocation
  void Cleanup()
  {
    WSACleanup();
  }

  //------------ Initialize WinSock Library
  bool StartSocket() {
    WORD Ver;
    WSADATA wsd;
    Ver = MAKEWORD( 2, 2 );
    if (WSAStartup(Ver,&wsd) == SOCKET_ERROR) {
	    WSACleanup();
	    return false;
    }
    return true;
  }

  //-----------------Get Last Socket Error
  int SocketGetLastError(){ return  WSAGetLastError();}
  //----------------- Close Socket
  int CloseSocket( SOCKET s ) { closesocket(s); return 0;}

  /* This is the critical section object (statically allocated). */
  CRITICAL_SECTION m_CriticalSection;

  void InitializeLock() {
	  InitializeCriticalSection(&m_CriticalSection);
  }
	
  void AcquireLock( ) {
    EnterCriticalSection(&m_CriticalSection);
  }

  void ReleaseLock() {
    LeaveCriticalSection( &m_CriticalSection );
  }
#else
  /** POSIX */
  typedef struct {
      int Sh; // Socket Handle which represents a Client
  } CLIENT_DATA;

  //-------------- Socket Descriptor is integer in POSIX
  int ListenerSocket;
  int InComingSocket;

  //----------------------- House Keeping Routines for GNU Linux
  void Cleanup() {} 
  bool StartSocket() { return true; }
  int SocketGetLastError() { return  0xFFFF; }

  #define SOCKET_ERROR (-1) 

  //------------------- Close Sockets
  int CloseSocket( int s ) { 
      shutdown(s, 2);
      return 0;
  }

  /* This is the critical section object (statically allocated). */
  static pthread_mutex_t cs_mutex =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
  void InitializeLock() {}
  void AcquireLock( ) { pthread_mutex_lock( &cs_mutex ); }
  void ReleaseLock() { pthread_mutex_unlock( &cs_mutex ); }

  #define INVALID_SOCKET (-1)
  #define DWORD unsigned long
  #define SOCKET int

  //--------------- Thread Routines
  void * ClientThreadProc(void * lpParam);
  void * ListenThreadProc(void * lpParameter);
#endif

#include "Server.h"
DWORD  CServerSocket::Thid =0;
struct sockaddr_in CServerSocket::m_RemoteAddress{};
SOCKET CServerSocket::m_ListnerSocket = -1;

/**
 * Listener thread procedure for accepting new connections and 
 * initiating the file transfer process
*/
#ifdef WINDOWS_OS
  DWORD WINAPI CServerSocket::ListenThreadProc(LPVOID lpParameter)
#else
  void * CServerSocket::ListenThreadProc(void * lpParameter)
#endif
{
    printf("Entered the Listener Thread....\n");
    while (1) {
        // Listen for new connections and accept connections
        unsigned int Len = sizeof(CServerSocket::m_RemoteAddress);
        #ifdef WINDOWS_OS
            InComingSocket = accept(
            CServerSocket::m_ListnerSocket,
            (struct sockaddr *) &CServerSocket::m_RemoteAddress, (int *)&Len);
        #else
            InComingSocket = accept(
            CServerSocket::m_ListnerSocket,
            (struct sockaddr *) &CServerSocket::m_RemoteAddress, (socklen_t *) &Len);
        #endif
      
        printf("....................After the Accept.................\n");
        if (InComingSocket == INVALID_SOCKET) {
            fprintf(stderr, "accept error %d\n", SocketGetLastError());
            Cleanup();
            return 0;
        }

        /**
         * Create a separate thread for handling the file transfer
         * procedure.
        */
        printf("....................Accepted a new Connection........\n");
        CLIENT_DATA ClientData;
        #ifdef WINDOWS_OS
            DWORD ThreadId;
            ClientData.Sh = InComingSocket;
            ::CreateThread(NULL, 0, CServerSocket::ClientThreadProc, (LPVOID)&ClientData, 0, &ThreadId);
        #else 
            pthread_t thread2;
            ClientData.Sh = InComingSocket;
            pthread_create( &thread2, NULL, CServerSocket::ClientThreadProc, (void*) &ClientData);
        #endif
  }

  return 0;
}

/**
 * Helper function to receive bytes from a socket
 * and move it to a buffer
 */
bool ReadSocketBuffer( SOCKET s, char *bfr, int size) {
    int RetVal = recv(s, bfr, size, 0);
    if (RetVal == 0 || RetVal == -1)
        return false;
    return true;
}

/**
 * Helper function to send acknowledgement
 * packet to a socket
 */
bool SendAcknowledgement(SOCKET s ) {
  T_ACK ack = MakeAck();
  int bytes_send = send( s, (char *)&ack, sizeof(ack),0);
  return bytes_send > 0 ;
}

/**
 * Send EOF packet to a socket
 */
bool SendEOF(SOCKET s ) {
  T_FILE_EOF eof = MakeEof( );
  int bytes_send = send( s, (char *)&eof, sizeof(eof),0);
  return bytes_send > 0 ;
}

#ifdef WINDOWS_OS
DWORD WINAPI CServerSocket::ClientThreadProc(LPVOID lpParam)
#else
void * CServerSocket::ClientThreadProc(void * lpParam)
#endif
{
    InitializeLock();
    printf("Acquired the Lock....\n");

    CLIENT_DATA CData;
    memcpy(&CData, lpParam, sizeof(CLIENT_DATA));

    char bfr[32000];
    memset(bfr, 0, 32000);

    // Receive data from the socket
    int RetVal = recv(CData.Sh, bfr, sizeof(bfr), 0);
    if (RetVal == 0 || RetVal == -1)
        return 0;

    // Get the packet type
    int packet_type = *((int *) bfr);
    printf("%d\n", packet_type);

    // Check if the first packet is handshake packet, else throw error
    if (packet_type != 1) {
        cout << "could not receive the right packet" << endl;
        return 0;
    }

    // Since we have received the handshake packet, send the acknowledgment packet
    T_ACK ack = MakeAck();
    int bytes_send = send(CData.Sh, (char *)&ack, sizeof(ack), 0);
    cout << "Finished Sending the Acknowledgement ... bytes send= " << bytes_send << endl;

    // Read from the client socket connection
    if (!ReadSocketBuffer(CData.Sh, bfr, sizeof(bfr))) {
        cout << "Failed to Read From Socket........" << "Exiting the App" << endl;
        return 0;
    }

    // Check if the packet type send is of type file metadata
    if (*((int *) bfr) != 3) {
        cout << "Expected Meta Data .... " << endl;
        return 0;
    }

    // Read file_meta_data from buffer
    T_FILE_META file_meta_data;
    memcpy(&file_meta_data, bfr, sizeof(file_meta_data));

    // Send acknowledgment
    cout << " file name = " << file_meta_data.file_name << " size = " << file_meta_data.size;
    SendAcknowledgement(CData.Sh);

    cout << "finished .....sending acknowledgement....." << "transferring files" << endl;
    cout << "Waiting for the content " << endl;

    // Open / Create a file in write binary mode
    FILE *fp = fopen("DEST_WIRE.out", "wb");

    // Loop and receive the file data chunks till we receive EOF packet
    int chunk_size = sizeof(T_FILE_CHUNK) + 5000;
    char buffer_chunk[chunk_size];
    int transferred_size = 0;
    while (1) {
        // Read from socket and move the bytes to buffer
        // cout << "Transferred = " << transferred_size << endl;
        memset(buffer_chunk, 0, sizeof(buffer_chunk));
        ReadSocketBuffer(CData.Sh, buffer_chunk, sizeof(buffer_chunk));
        cout << "Packet Type = " << *(int *) buffer_chunk << endl;

        // Check if packet_type is EOF, and close the file pointer and thread if it so.
        if (*(int *) buffer_chunk == 5) {
            cout << "End of File Received" << endl;
            fclose(fp);

            // Send acknowledment to client 
            SendAcknowledgement(CData.Sh);
            if (transferred_size != file_meta_data.size) {
                cout << "Expected Size = " << file_meta_data.size << endl;
                cout << "Received Size = " << transferred_size << endl;
            }
            return 0;
        }

        // Check if the packet type is of file chunk, terminate if it is not
        if (*(int *) buffer_chunk != 4) {
            cout << "I do not know what happens here" << endl;
            cout << "What kind of packet = " << *(int *) buffer_chunk << endl;
            fclose(fp);
            return 0;
        }

        // Receive the file chunk, copy to temporary memory, and write it to desination file.
        T_FILE_CHUNK *chunk = (T_FILE_CHUNK *) buffer_chunk;
        char temp_mem[32000];
        // cout << "Received Block ...... " << chunk->buffer_size << endl;

        cout << "Packet sequence ...... " << chunk->packet_seq_num << endl;
        if (chunk->packet_seq_num > 10) {
            return 0;
        }
        memcpy(temp_mem, chunk->buffer, chunk->buffer_size);
        fwrite(temp_mem, 1, chunk->buffer_size, fp);
        transferred_size += chunk->buffer_size;
    }

    return 0;
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stdout, "Usage: Server <portid> \n");
        return 1;
    }

    if (!StartSocket()) {
        fprintf(stdout, "Failed To Initialize Socket Library\n");
        return 1;
    }

    // Initialize the class used for socket communication
    CServerSocket sock{atoi(argv[1])};

    // Open the port for socket communication
    sock.Open();

    while (1);

    Cleanup();
    return 0;
}
