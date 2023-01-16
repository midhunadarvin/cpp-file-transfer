/**
 * Payload.h : Defines the enums and packet structures for Wire level protocol for the File transfer
 *
 * Wire protocol is used to describe a common way to exchange information at the application level. It refers to an application layer protocol
 * and defines all the required attributes for the data exchange, like data types (units of data, message formats, etc.), communication endpoints and
 * capabilities (such as delivery guarantees, direction of communication, etc.).
 */
#ifndef _PAYLOAD_DOT_H
#define _PAYLOAD_DOT_H

/**
 * Packet Types for the Wire Level Protocol for the FILE transfer
 */
enum PacketType
{
    CHISTA_NULL = -1,
    CHISTA_HAND_SHAKE = 1,      // sends "hello"
    CHISTA_ACK = 2,             // "olleh"
    CHISTA_META_DATA = 3,       // sends file_name and size
    CHISTA_FILE_CONTENT = 4,    // sends chunks of 4096
    CHISTA_META_EOF = 5,        // sends the End of File Token
    CHISTA_META_FATAL_ERROR = 6 // sends the Fatal Error message
};

typedef struct
{
    int packet_type;
    char hello_str[6];
    char padding[2];
} T_HAND_SHAKE;

typedef struct
{
    int packet_type;
    char hello_str[6];
    char padding[2];
} T_ACK;

typedef struct
{
    int packet_type;
    char file_name[255];
    long size;
    char padding[1];
} T_FILE_META;

typedef struct
{
    int packet_type;
    int packet_seq_num;
    int buffer_size;
    char buffer[1];
} T_FILE_CHUNK;

typedef struct
{
    int packet_type;
    char eof_str[4];
} T_FILE_EOF;

typedef struct
{
    int packet_type;
    char err_msg[1020];
} T_FILE_FATAL_ERROR;

/**
 * Generates the file metadata packet struct
*/
T_FILE_META MakeFileMeta(char *file_name, long size);

/**
 * Generates the handshake packet struct
*/
T_HAND_SHAKE MakeHandShake();

/**
 * Generates the acknowledgement packet struct
*/
T_ACK MakeAck();

/**
 * Generates the EOF (end of file) packet struct
*/
T_FILE_EOF MakeEof();

/**
 * Generates the Buffer packet struct
*/
T_FILE_CHUNK *MakeBufferPack(void *buffer, int len, int);

/**
 * Generates the Error packet struct
*/
T_FILE_FATAL_ERROR MakeErrorPack(char *buffer, int len);

/**
 * Computes the file size of the input file 
*/
long ComputeFileSize(char *file_name);

#endif
