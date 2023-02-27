#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <cstdlib>

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <memory.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

#include "defaults.hpp"
#include "structure.hpp"

using namespace std;

void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress, bool& debugFlag, uint32_t& msDelay, uint32_t& numDG);
void DisplayMenu(char* argv[]);
inline string ClientDatagramToString(ClientDatagram* p_cd, bool isInNetworkOrder);
inline string ServerDatagramToString(ServerDatagram* p_sd, bool isInNetworkOrder);

#define RECV_ATTEMPT_COUNT 9

#define INVALID_OPTION 9
#define HELP_MENU_RV 10
#define SOCKET_ERROR_RV 11
#define SERVER_CONNECTION_ERROR_RV 12
#define MESSAGE_SEND_ERROR_RV 13
#define MESSAGE_RECV_ERROR_RV 14
#define FCNTL_ERROR_RV 15
#define OB_MALLOC_FAIL_RV 16
#define IB_MALLOC_FAIL_RV 17

#define MESSAGE "jball3"

int main(int argc, char * argv[])
{
	int retval = 0; // 0 means successful run, otherwise some error
	int udpSocketNumber; // Will be the socket FD we use


	bool isDebugMode = false;
	uint32_t microsecDelay = 0;
	int serverPort = PORT_NUMBER; // The port number of the server
	string serverAddress = SERVER_IP; // The IP address of the server
	uint32_t numDatagrams = NUMBER_OF_DATAGRAMS; // How many datagrams we want this client to send

	struct sockaddr_in serverSockAddr; // The Server connection socket information
	struct hostent* serverHostEnt; // The actual server connection information
	ssize_t bytesReceived; // Will keep track of how many bytes we receive FROM the server
	ssize_t bytesSent; // Will keep track of how many bytes we send to the server
	socklen_t incomingSocketLength = sizeof(serverSockAddr);

	// Some vital size info for our datagrams
	const size_t MESSAGE_SIZE = strlen(MESSAGE) + 1; // +1 for the null terminator
	const ssize_t EXP_OUTGOING_DATAGRAM_SIZE = sizeof(ClientDatagram) + MESSAGE_SIZE; // Expected  size of the outgoing datagram's metadata and message
	const ssize_t EXP_INCOMING_DATAGRAM_SIZE = sizeof(ServerDatagram); // Expected size of the incoming datagram from the server

	// Construction Buffers
	char* outgoingBuffer = nullptr; // Used to build and hold the outgoing datagram and message
	char* incomingBuffer = nullptr; // Used to hold the incoming datagram from the server

	// Statistics variables
	ssize_t numPacketsSent = 0; // Keep track of how many packets the client sent to the server
	set<uint32_t> seqNumbersSent; // Keep track of which packet numbers leave and come back
	set<uint32_t>::iterator setIT; // Intializing the iterator here makes it faster later on

	try
	{
		GetOptions(argc, argv, serverPort, serverAddress, isDebugMode, microsecDelay, numDatagrams);
		cout << "Datagram length: " << sizeof(ClientDatagram) << " size of string: " << MESSAGE_SIZE << " total size: " << EXP_OUTGOING_DATAGRAM_SIZE << endl;

		#pragma region ConnectionSetup

		// Create our outgoing data socket
		if(isDebugMode) cout << "Opening socket..." << endl;
		udpSocketNumber = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpSocketNumber < 0)
		{
			perror("ERROR opening socket");
			throw SOCKET_ERROR_RV;
		}

		// Get our server's connection info
		if(isDebugMode) cout << "Obtaining server connection info..." << endl;
		cout << "Client attempting to connect to ip: " << serverAddress << endl;

		serverHostEnt = gethostbyname(serverAddress.c_str());
		if(serverHostEnt == nullptr)
		{
			cerr << "ERROR, no such host: " << serverAddress << endl;
			throw SERVER_CONNECTION_ERROR_RV;
		}

		// Configure our connection settings
		if(isDebugMode) cout << "Configuring connection settings..." << endl;
		cout << "Client attempting to connect to port: " << serverPort << endl;

		memset(&serverSockAddr, 0, sizeof(serverSockAddr)); // Wipe out any garbage values
		serverSockAddr.sin_family = AF_INET;
		memmove(&serverSockAddr.sin_addr.s_addr, serverHostEnt->h_addr, serverHostEnt->h_length);
		serverSockAddr.sin_port = htons(serverPort);

		// SET SOCKET TO NON_BLOCKING IO
		int retV = fcntl(udpSocketNumber, F_SETFL, O_NONBLOCK); // Set our socket to NonBlocking
		if(retV == -1)
		{
			perror("ERROR, fcntl():");
			throw FCNTL_ERROR_RV;
		}

		#pragma endregion

		// Allocate our buffers on the heap to be used later
		// Doing this before out datagrams go out helps with efficiency
		outgoingBuffer = (char*)malloc(EXP_OUTGOING_DATAGRAM_SIZE);
		if(outgoingBuffer == nullptr)
		{
			cerr << "Failed to allocate outgoing buffer" << endl;
			throw OB_MALLOC_FAIL_RV;
		}
		memset(outgoingBuffer, 0, EXP_OUTGOING_DATAGRAM_SIZE); // Get rid of any garbage values

		incomingBuffer = (char*)malloc(EXP_INCOMING_DATAGRAM_SIZE);
		if(incomingBuffer == nullptr)
		{
			cerr << "Failed to allocate incoming buffer" << endl;
			throw IB_MALLOC_FAIL_RV;
		}
		memset(incomingBuffer, 0, EXP_INCOMING_DATAGRAM_SIZE); // Get rid of any garbage values


		/*
			BUILDING THE BASICS FOR OUTGOING DATAGRAM
		*/
		ClientDatagram* outgoingDG = (ClientDatagram*)outgoingBuffer;

		// Set payload length
		outgoingDG->payload_length = htons(MESSAGE_SIZE);
		outgoingDG->sequence_number = 0;

		// Copy our message after the datagram
		// (char*)(outgoingDG + 1) will return the address right after the datagram
		strncpy((char*)(outgoingDG + 1), MESSAGE, MESSAGE_SIZE); // Copy the message after the datagram metadata

		// We wanna send a certain number of datagrams out
		for(size_t dgNum = 0; dgNum < numDatagrams; dgNum++)
		{
			// Set our new sequence number
			outgoingDG->sequence_number = htonl(dgNum);

			/*
				SENDING THE DATAGRAM
			*/
			bytesSent = sendto(udpSocketNumber, outgoingBuffer, EXP_OUTGOING_DATAGRAM_SIZE, 0, (struct sockaddr*) &serverSockAddr, sizeof(serverSockAddr));

			// Check if there was an error with sending
			if(bytesSent != EXP_OUTGOING_DATAGRAM_SIZE)
			{
				cerr << "Sent: " << bytesSent << " expected to send: " << EXP_OUTGOING_DATAGRAM_SIZE << endl;
				perror("ERROR, sendto()");
			}
			else
			{
				// Our packet was sucessfully sent
				if(isDebugMode) cout << "Sent datagram to server: " << ClientDatagramToString(outgoingDG, true) << endl;
				numPacketsSent++; // Another happy landing!
				// We want to store the non-byte-ordered sequence number for use later
				seqNumbersSent.insert(ntohl(outgoingDG->sequence_number));
			}

			// Delay before the next outgoing datagram if we want to
			if(usleep(microsecDelay) == -1)
			{
				perror("ERROR, usleep():");
			}

			/*
			=======================
				RECIEVING
			=======================
			*/
			for(int recvAttempt = 0; recvAttempt < RECV_ATTEMPT_COUNT; recvAttempt++)
			{
				bytesReceived = recvfrom(udpSocketNumber, incomingBuffer, EXP_INCOMING_DATAGRAM_SIZE, 0, (struct sockaddr *) &serverSockAddr, &incomingSocketLength);

				if(bytesReceived > 0)
				{
					// We recieved some data, so let's try to interpret it
					ServerDatagram* incomingDG = (ServerDatagram*)(incomingBuffer);
					incomingDG->datagram_length = ntohs(incomingDG->datagram_length); // network to host short
					incomingDG->sequence_number = ntohl(incomingDG->sequence_number); // network to host long

					// Print out info alerting us to a datagram reception
					if(isDebugMode)
					{
						cout << "Recieved datagram from server: " << ServerDatagramToString(incomingDG, false) << endl;
					}


					// Check if the recieved sequence number is in our set of send sequence numbers...
					if((setIT = seqNumbersSent.find(incomingDG->sequence_number)) != seqNumbersSent.end())
					{
						// The packet we recieved was one we sent out previously
						// So remove it from our list of outgoing
						seqNumbersSent.erase(setIT);
					}
					else
					{
						// Unknown packet recieved
						cerr << "ERROR: Unknown sequence number recieved: " << incomingDG->sequence_number << endl;
						// We don't want to stop here since we may still recieve valid ones
					}

				}
				else
				{
					// Something happened and no data was recieved

					// 	These flags are set when there was nothing to read
					if(errno == EAGAIN || errno == EWOULDBLOCK)
					{
						continue; // We just wanna move on to the next datagram
					}

					if(bytesReceived == 0)
					{
						cout << "Recieved zero bytes -- If you see this, something very weird happened!" << endl;
					}
					else
					{
						// We have a real error at this point
						perror("ERROR, recvfrom()");
					}
				}
			}
		}

	}
	catch (int rv)
	{
		retval = rv;
	}

	// Let's print out the stats from our interaction
	// - Total number of messages sent
	cout << numPacketsSent << " messages sent!" << endl;
	// - How many did we not get back?
	cout << "Unacknowledged packets: " << seqNumbersSent.size() << endl;

	// CLEANUP TIME
	// Close our socket
	if(udpSocketNumber >= 0)
	{
		close(udpSocketNumber);
	}

	// Deallocate our incoming data buffer
	if(incomingBuffer != nullptr)
	{
		free(incomingBuffer);
		incomingBuffer = nullptr;
	}

	// Deallocate our outgoing data buffer
	if(outgoingBuffer != nullptr)
	{
		free(outgoingBuffer);
		outgoingBuffer = nullptr;
	}

	return retval;
}

void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress, bool& debugFlag, uint32_t& msDelay, uint32_t& numDG)
{
	int c;
	while ((c = getopt(argc, argv, "hs:p:dy:n:")) != -1)
	{
		switch (c)
		{
			case ('h'):
			{
				// -h prints help information and exits.
				DisplayMenu(argv);
				throw HELP_MENU_RV;
			}
			case('y'):
			{
				// -y (number) introduces a delay between sending datagrams
				msDelay = atoi(optarg);
				break;
			}
			case('n'):
			{
				// -n (number) overrides the default number of datagrams to send
				numDG = atoi(optarg);
				break;
			}
			case ('s'):
			{
				// -s (serverIP) allows you to override the default of 127.0.0.1.
				serverAddress = string(optarg);
				break;
			}
			case ('p'):
			{
				// -p (port) allows you to override the default destination port of 39390.
				portNumber = atoi(optarg);
				break;
			}
			case ('d'):
			{
				// -d enables debugging
				debugFlag = true;
				break;
			}
			default:
			{
				cerr << "ERROR, Invalid option [" << c << "]" << endl;
				throw INVALID_OPTION;
			}
		}
	}
}
//--
void DisplayMenu( char* argv[])
{
	cout << argv[0] << " options:" << endl;
	cout << "  -h displays help" 	<< endl;
	cout << "  -d enables debug mode" << endl;
	cout << setfill('.') << setiosflags(ios_base::left) << setw(45) << "  -s server_address " 				<< "defaults to "  	<< SERVER_IP 			<< endl;
	cout << setfill('.') << setiosflags(ios_base::left) << setw(45) << "  -p port_number " 					<< "defaults to " 	<< PORT_NUMBER 			<< endl;
	cout << setfill('.') << setiosflags(ios_base::left) << setw(45) << "  -y overrides delay (in microseconds) "	<< "defaults to "	<< 0 					<< endl;
	cout << setfill('.') << setiosflags(ios_base::left) << setw(45) << "  -n overrides number of datagrams " 	<< "defaults to " 	<< NUMBER_OF_DATAGRAMS 	<< endl;
}
//--
inline string ClientDatagramToString(ClientDatagram* p_cd, bool isInNetworkOrder = false)
{
	return string(
		"ClientDatagram | seqnum: " + to_string( isInNetworkOrder ? ntohl(p_cd->sequence_number) : p_cd->sequence_number)
		+ " pl: " + to_string( isInNetworkOrder ? ntohl(p_cd->payload_length) : p_cd->payload_length)
		+ " m: " + ((char*)p_cd + sizeof(ClientDatagram)));
}
//--
inline string ServerDatagramToString(ServerDatagram* p_sd, bool isInNetworkOrder = false)
{
	return string(
		"Server Datagram | seqnum: " + to_string( isInNetworkOrder ? ntohl(p_sd->sequence_number) : p_sd->sequence_number)
		+ " dl: " + to_string( isInNetworkOrder ? ntohs(p_sd->datagram_length) :p_sd->datagram_length)
	);
}
