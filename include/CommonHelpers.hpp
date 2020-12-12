#ifndef   FILE_H
#define   FILE_H

#include "InterruptException.hpp"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
// socklen_t and others
#include <sys/socket.h> 

// for memcopy
#include <stdio.h>
#include <string.h>

#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <sstream>

// for streams - cout, cerr, etc.
#include <iostream>

#define PORT 4410
#define BACKLOG 10
#define QUEUE_SIZE 25
#define MAX 1024
#define RECVFRAME 30

using namespace std;


/**
 * I first tried making the frame a class but sizeof(Class) did not work
 * My understanding is there is a difference between size of a base class vs derived
 * also copying the buffer into it did not work, struct was simpler so I went with struct
 * https://www.includehelp.com/cpp-tutorial/size-of-a-class-in-cpp-padding-alignment-in-class-size-of-derived-class.aspx 
 */
struct Frame
{
	int sequence_number = -1;
	int source_address = -1;
	int destination_address = -1;
	char data[MAX] = { ' ' };
};


static const char* ZEUS_IP = "147.26.231.156";
static const char* EROS_IP = "147.26.231.153";

static const char NEW[] = "New";
static const char REQUEST[] = "Request";
static const char POSITIVE[] = "Positive";
static const char SEND[] = "Send";
static const char RECEIVE[] = "Receive";
static const char NEGATIVE[] = "Reject";

// the size of the frame is 24; instead of hardcoding we obtain dynamically
static const int FRAME_SIZE = sizeof(struct Frame);


void save_buff(int seq_num, int src, int dest, const char* word, char* buf)
{
	struct Frame frame;
	frame.sequence_number = seq_num;
	frame.source_address = src;
	frame.destination_address = dest;
	
	// memcpy signature:
	// void *memcpy(void *dest, const void * src, size_t n)
	memcpy(frame.data, word, sizeof(frame.data));
	memcpy(buf, &frame, FRAME_SIZE);
}

void read_buffer(struct Frame* frame, char* buf)
{
	memcpy(frame, buf, FRAME_SIZE);
}

/**
* err_sys is used in the book "UNIX Network Programming: The sockets networking API" by Richard Stevens. 
* I will use it here with a slight modification, adding an argument for the code.
* This way we can have a unique error code
* 
* Error codes: 
* -1 Write on socket error
* -2 Write on socket error
* -3 Open file error
* -4 Error creating socket
* -5 Error connecting on socket
* -6 Too many clients
* -7 Error accepting 
* -9 Wrong # of arguments 
* -10 Other (e.g., select() error)
*/
void err_sys(string str, int code)
{
	stringstream ss;
	ss << str << " , Err_Code: " << code;
    perror(ss.str().c_str());
    exit(code);
}

/**
 * Just a small helper function to make things more appealing 
 * for viewing on the command line. 
 * Prints out CS=5310, the course number
 */
void print_art()
{
	char art[5][35]={
						"   ___ ___    ___ ____ _  __  ",
						"  / __/ __|  | __|__ /| |/  \\ ",
						" | (__\\__ \\==\\__ \\|_ \\| | () |",
						"  \\___|___/  |___/___/|_|\\__/ ",  
						"                                "     
					};

			int height = 0;
            int row = 0;  
            while(height < 5)  
            {
				cout << art[row++] << endl;
				height++;
            }
}

#endif
