/**
 * Server.h : Defines the class for socket connection and communication used in the server side
 */
#ifndef _SERVER_DOT_H
#define _SERVER_DOT_H

class CServerSocket
{
   int m_ProtocolPort = 3500;
   struct sockaddr_in m_LocalAddress;

public:
   static struct sockaddr_in m_RemoteAddress;
   char m_Buffer[1024];
   static SOCKET m_ListnerSocket;
   static DWORD Thid;

   #ifdef WINDOWS_OS 
      static DWORD WINAPI ListenThreadProc(LPVOID lpParameter);
      static DWORD WINAPI ClientThreadProc(LPVOID lpParameter);
   #else
      static void * ListenThreadProc(void * lpParameter);
      static void * ClientThreadProc(void * lpParameter);
   #endif 

   /**
    * Constructor
    * 
    * @param p_port The port to which the socket should bind
   */
   CServerSocket(int p_port)
   {
      // Initialize the m_LocalAddress and m_ProtocolPort
      // int socket(int domain, int type, int protocol)
      // AF_INET      --> ipv4
      // SOCK_STREAM  --> TCP
      // SOCK_DGRAM   --> UDP
      // protocol = 0 --> default for TCP
      m_ProtocolPort = p_port;
      m_LocalAddress.sin_family = AF_INET;
      m_LocalAddress.sin_addr.s_addr = INADDR_ANY;
      m_LocalAddress.sin_port = htons(m_ProtocolPort); // converts the integer from host byte order to network byte order.
   }

   /**
    * Opens the socket, binds to the local address and port and starts listening for incoming messages
   */
   bool Open() {
      // open socket
      if ((m_ListnerSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
         return false;
      }

      // Bind to local ip address and port 
      printf("%s\n", "About to Bind.. ................ ");
      if (bind(m_ListnerSocket, (struct sockaddr *)&m_LocalAddress, sizeof(m_LocalAddress)) == SOCKET_ERROR) {
         return false;
      }

      // Listen for incoming messages
      printf("%s\n", "Listen. ................ ");
      if (listen(m_ListnerSocket, 5) == SOCKET_ERROR) {
         return false;
      }

      // Creates a separate thread for listening to client messages
      StartListeningThread();

      return true;
   }

   /**
    * Starts a separate thread for listening for incoming messages
    */
   bool StartListeningThread() {
      #ifdef WINDOWS_OS
         printf("%s\n", "Creating a  Windows Thread....... for Listener\n");
         CreateThread(NULL, 0, ListenThreadProc, NULL, 0, &Thid);
      #else
         pthread_t thread1;
         int iret1;
         printf("%s\n", "Creating a POSIX Thread....... for Listener\n");
         iret1 = pthread_create(&thread1, NULL, ListenThreadProc, (void*) 0);
      #endif
      return true;
   }

   /**
    * Closes the socket connection
   */
   bool Close() {
      #ifdef WINDOWS_OS
        closesocket(m_ListnerSocket);
        return false;
      #else
        shutdown(m_ListnerSocket, 2);
        return 0;
      #endif
   }
};

#endif
