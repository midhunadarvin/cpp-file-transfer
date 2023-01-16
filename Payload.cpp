/**
 * Payload.cpp : Contains the definitions of the functions declared in Payload.h
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
#include <string.h>           // contains helper functions for string operations
#include <stdio.h>            // contains declarations and functions to help with input output operations
#include <iostream>           // contains declarations and functions to help with input output operations
#include <fstream>            // predefines a set or operations for handling files related to input and output.

#include "Payload.h"          // Defines the enums and packet structures for Wire level protocol

using namespace std;

/**
 * Generates a PDU (Protocol Data Unit) for transferring file meta information
 * https://www.geeksforgeeks.org/protocol-data-unit-pdu/
*/
T_FILE_META MakeFileMeta(char *file_name, long size)
{
    T_FILE_META return_value;

    // set memory of return value and initialize as 0
    memset(&return_value, 0, sizeof(T_FILE_META));

    // set struct values
    return_value.packet_type = CHISTA_META_DATA;
    strcpy(return_value.file_name, file_name);
    return_value.size = size;

    return return_value;
}

/**
 * Generates HandShake PDU ( sends string "hello" )
 */ 
T_HAND_SHAKE MakeHandShake()
{
    T_HAND_SHAKE ret_val;

    // set memory of return value and initialize as 0
    memset(&ret_val, 0, sizeof(T_HAND_SHAKE));

    // set struct values
    ret_val.packet_type = CHISTA_HAND_SHAKE;
    strcpy(ret_val.hello_str, "hello");

    return ret_val;
}

/**
 * Generates Acknowledgment PDU ( sends string "olleh" )
 */
T_ACK MakeAck()
{
    T_ACK ret_val;
    memset(&ret_val, 0, sizeof(T_ACK));
    ret_val.packet_type = CHISTA_ACK;
    strcpy(ret_val.hello_str, "olleh");
    return ret_val;
}

/**
 * Generates the End of File PDU
 */
T_FILE_EOF MakeEof()
{
    T_FILE_EOF ret_val;
    memset(&ret_val, 0, sizeof(T_FILE_EOF));
    ret_val.packet_type = CHISTA_META_EOF;
    strcpy(ret_val.eof_str, "EOF");
    return ret_val;
}

/**
 * Generates the File chunk PDU
 */
T_FILE_CHUNK *MakeBufferPack(void *buffer, int length, int seq_num)
{
    int actual_buffer_size = sizeof(T_FILE_CHUNK) + length + 4;

    // Initial file chunk and allocate memory
    T_FILE_CHUNK *return_value = (T_FILE_CHUNK *) malloc(actual_buffer_size);

    // clear memory
    memset(return_value, 0, actual_buffer_size);

    // set struct values
    return_value->packet_type = CHISTA_FILE_CONTENT;
    return_value->packet_seq_num = seq_num;
    return_value->buffer_size = length;
    memcpy(return_value->buffer, buffer, length);

    return return_value;
}

/**
 * Generates the Error packet PDU
 */
T_FILE_FATAL_ERROR MakeErrorPack(char *buffer, int len)
{
    T_FILE_FATAL_ERROR return_value;
    // Initialize
    memset(&return_value, 0, sizeof(T_FILE_FATAL_ERROR));

    // Set struct values
    return_value.packet_type = CHISTA_META_FATAL_ERROR;
    if (len < 1020) {
        strcpy(return_value.err_msg, buffer);
    } else {
        strcpy(return_value.err_msg, "Error Message Too Long...");
    }

    return return_value;
}

/**
 * Computes the file size for a given file name
 */
long ComputeFileSize(char *file_name)
{
    // input file stream constructor
    ifstream in_file (file_name, ios::binary);

    // seekg() is a function in the iostream library that allows you to seek an arbitrary position in a file. seeks to the end.
    in_file.seekg(0, ios::end);

    // gives us the current position of the file ( ie. the file size )
    int file_size = in_file.tellg();
    
    return (long) file_size;
}
