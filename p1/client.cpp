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

#define INVALID_OPTION 9
#define HELP_MENU_RV 10
#define SOCKET_ERROR_RV 11
#define SERVER_CONNECTION_ERROR_RV 12
#define MESSAGE_SEND_ERROR_RV 13
#define MESSAGE_RECV_ERROR_RV 14;

#define MESSAGE "jball3"

int main(int argc, char * argv[])
{
	int retval = 0;
	int udpSocketNumber;
	bool isDebugMode = false;
	uint32_t microsecDelay = 0;

	int serverPort = PORT_NUMBER;
	string serverAddress = SERVER_IP;
	uint32_t numDatagrams = NUMBER_OF_DATAGRAMS;

	struct sockaddr_in serverSockAddr;
	struct hostent* serverHostEnt;
	ssize_t bytesReceived;
	socklen_t incomingSocketLength = sizeof(serverSockAddr);

	const size_t MESSAGE_SIZE = strlen(MESSAGE) + 1; // +1 for the null terminator
	const ssize_t EXP_OUTGOING_DATAGRAM_SIZE = sizeof(ClientDatagram) + MESSAGE_SIZE; // Expected  size of the outgoing datagram's metadata and message
	const ssize_t EXP_INCOMING_DATAGRAM_SIZE = sizeof(ServerDatagram); // Expected size of the incoming datagram from the server
	char* outgoingBuffer = nullptr;
	char* incomingBuffer = nullptr;
	try
	{
		GetOptions(argc, argv, serverPort, serverAddress, isDebugMode, microsecDelay, numDatagrams);

		#pragma region ConnectionSetup
		// Create our outgoing data socket
		if(isDebugMode) cout << "Opening socket..." << endl;
		udpSocketNumber = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpSocketNumber < 0)
		{
			perror("ERROR opening socket");
			throw SOCKET_ERROR_RV;
		}

		fcntl(udpSocketNumber, F_SETFL, O_NONBLOCK); // Set our socket to NonBlocking

		// Get our server's connection info
		if(isDebugMode) cout << "Obtaining server connection info..." << endl;
		serverHostEnt = gethostbyname(serverAddress.c_str());
		if(serverHostEnt == nullptr)
		{
			close(udpSocketNumber);
			cerr << "ERROR, no such host: " << serverAddress << endl;
			throw SERVER_CONNECTION_ERROR_RV;
		}

		// Configure our connection settings
		if(isDebugMode) cout << "Configuring connection settings..." << endl;
		memset(&serverSockAddr, 0, sizeof(serverSockAddr));
		serverSockAddr.sin_family = AF_INET;
		memmove(&serverSockAddr.sin_addr.s_addr, serverHostEnt->h_addr, serverHostEnt->h_length);
		serverSockAddr.sin_port = htons(serverPort);

		#pragma endregion


		// We wanna send a certain number of datagrams out
		for(size_t dgNum = 0; dgNum < numDatagrams; dgNum++)
		{
			/*
				BUILDING THE DATAGRAM
			*/
			if(isDebugMode) cout << "Creating datagram..." << endl;
			outgoingBuffer = (char*)malloc( EXP_OUTGOING_DATAGRAM_SIZE );

			ClientDatagram* outgoingDG = (ClientDatagram*)outgoingBuffer;

			// Set payload length
			outgoingDG->payload_length = MESSAGE_SIZE;
			outgoingDG->sequence_number = dgNum;

			strcpy(outgoingBuffer + sizeof(ClientDatagram), MESSAGE); // Copy the message after the datagram metadata

			if(isDebugMode)
			{
				cout << "Datagram created!";
				cout << " sn: " 	<< outgoingDG->sequence_number  << " {" 	<< sizeof(outgoingDG->sequence_number)	<< " bytes}";
				cout << " pl: " 	<< outgoingDG->payload_length	<< " {" 	<< sizeof(outgoingDG->payload_length) 	<< " bytes}";
				cout << " m:  " 	<< MESSAGE					 	<< " {" 	<< MESSAGE_SIZE							<< " bytes}";
				cout << " sz: " 	<< EXP_OUTGOING_DATAGRAM_SIZE 	<< " bytes" << endl;
			}

			// Prepare ByteOrdering of Datagram for network
			outgoingDG->payload_length = htons(outgoingDG->payload_length);
			outgoingDG->sequence_number = htonl(outgoingDG->sequence_number);

			/*
				SENDING THE DATAGRAM
			*/
			if(microsecDelay > 0) usleep(microsecDelay); // Delay before the next outgoing datagram if we want to

			if(isDebugMode) cout << "Attempting to send payload..." << endl;
			ssize_t bytesSent = sendto(udpSocketNumber, outgoingBuffer, EXP_OUTGOING_DATAGRAM_SIZE, 0, (struct sockaddr*) &serverSockAddr, sizeof(serverSockAddr));
			// Check if there was an error with sending
			if(bytesSent != EXP_OUTGOING_DATAGRAM_SIZE)
			{
				cerr << "Sent: " << bytesSent << " expected to send: " << EXP_OUTGOING_DATAGRAM_SIZE << endl;
				perror("sendto()");
				throw MESSAGE_SEND_ERROR_RV;
			}

			free(outgoingBuffer);
			outgoingBuffer = nullptr;
			outgoingDG = nullptr;

			/*
			=======================
				RECIEVING
			=======================
			*/

			incomingBuffer = (char*) malloc( EXP_INCOMING_DATAGRAM_SIZE );
			bytesReceived = recvfrom(udpSocketNumber, incomingBuffer, EXP_INCOMING_DATAGRAM_SIZE, 0, (struct sockaddr *) &serverSockAddr, &incomingSocketLength);

			if(errno == EAGAIN || errno == EWOULDBLOCK)
			{
				// These flags are set when there was nothing to read
				free(incomingBuffer);
				incomingBuffer = nullptr;
				continue; // We just wanna move on to the next datagram
			}


			if(bytesReceived > 0)
			{
				// We recieved something
				ServerDatagram* incomingDG = (ServerDatagram*)(incomingBuffer);
				incomingDG->datagram_length = ntohs(incomingDG->datagram_length); // network to host short
				incomingDG->sequence_number = ntohl(incomingDG->sequence_number); // network to host long

				// Print out info alerting us to a datagram reception
				cout << "Recieved datagram from server: ";
				cout << "     seq: " << incomingDG->sequence_number << " len: " << incomingDG->datagram_length << endl;
			}
			else
			{
				// We have a real error at this point
				perror("recvfrom()");
				throw MESSAGE_RECV_ERROR_RV;
			}

			free(incomingBuffer);
			incomingBuffer = nullptr;
		}

	}
	catch (int rv)
	{
		retval = rv;
	}

	if(udpSocketNumber >= 0)
	{
		close(udpSocketNumber);
	}

	if(incomingBuffer != nullptr)
	{
		free(incomingBuffer);
		incomingBuffer = nullptr;
	}

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
				numDG = 5; // For debugging...set only 5 datagrams (will be overrided if -n is supplied)
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
	cerr << argv[0] << " options:" << endl;
	cerr << "	-h displays help" << endl;
	cerr << "	-s server_address (required)" << endl;
	cerr << "	-p port_number    (required)" << endl;
	cerr << "   -m message        (required)" << endl;
	cerr << "   -n datagram_count" << endl;
	cerr << "   -y delay_in_ms" << endl;
}
