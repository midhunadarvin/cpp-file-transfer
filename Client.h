/**
 * Client.h : Defines the class for socket connection and communication used in the client side
 */
#ifndef _CLIENT_DOT_H
#define _CLIENT_DOT_H

class CClientSocket
{
    char m_ServerName[255];
    int m_PortNumber;
    struct sockaddr_in m_Server;
    struct hostent *m_HostPointer;
    unsigned int m_addr;
    SOCKET m_ConnectSock;

public:
    /**
     * Constructor
     * 
     * @param ServerName The address of the remote server
     * @param PortNumber The port to which the socket should bind
     */
    CClientSocket(char *ServerName, int PortNumber)
    {
        strcpy(m_ServerName, ServerName);
        m_PortNumber = PortNumber;
    }

    SOCKET GetSocket() { return m_ConnectSock; }

    /**
     * Resolves the sockaddr_in object
     */
    bool Resolve()
    {
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

    /**
     * Creates a socket and connects to the remote server
     */
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

    /**
     * Sends the data from the buffer to the socket
     */
    bool Send(void *buffer, int len)
    {
        int RetVal = send(m_ConnectSock, (const char *)buffer, len, 0);
        if (RetVal == SOCKET_ERROR)
        {
            return false;
        }

        return true;
    }

    /**
    * Receives the data from the socket and moves it to the buffer input
    */
    bool Receive(void *buffer, int *len)
    {
        int RetVal = recv(m_ConnectSock, (char *)buffer, *len, 0);

        if (RetVal == 0 || RetVal == -1)
        {
            printf("Error at socket(): %ld\n", SocketGetLastError());
            return false;
        }

        *len = RetVal;
        return true;
    }

    /**
     * Closes the socket connection
     */
    int Close()
    {
        CloseSocket(m_ConnectSock);
        return 0;
    }
};

#endif
