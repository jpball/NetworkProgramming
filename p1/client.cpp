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

void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress, bool& debugFlag, uint32_t& msDelay);
void DisplayMenu(char* argv[]);
const ClientDatagram BuildDatagram(char* buffer, uint32_t seqNum, char* message);
inline char* GetDatagramMessage(char* datagramBuffer);

#define BUFFER_SIZE 1024

#define INVALID_OPTION 9
#define HELP_MENU_RV 10
#define SOCKET_ERROR_RV 11
#define SERVER_CONNECTION_ERROR_RV 12
#define MESSAGE_SEND_ERROR_RV 13
#define MESSAGE_RECV_ERROR_RV 14;

int main(int argc, char * argv[])
{
	int retval = 0;
	int udpSocketNumber;
	bool isDebugMode = false;
	uint32_t microsecDelay = 0;

	int serverPort = PORT_NUMBER;
	string serverAddress = SERVER_IP;
	ssize_t numDatagrams = NUMBER_OF_DATAGRAMS;

	struct sockaddr_in serverSockAddr;
	struct hostent* serverHostEnt;
	ssize_t bytesReceived;
	socklen_t incomingSocketLength = sizeof(serverSockAddr);

	char buffer[BUFFER_SIZE];
	try
	{
		GetOptions(argc, argv, serverPort, serverAddress, isDebugMode, microsecDelay);
		if(isDebugMode) numDatagrams = 5;

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


		// We wanna send a certain number of datagrams out
		for(size_t dgNum = 0; dgNum < numDatagrams; dgNum++)
		{
			/*
				BUILDING THE DATAGRAM
			*/
			if(isDebugMode) cout << "Creating datagram..." << endl;
			char message[] = "HelloWorld";
			size_t messageSize = strlen(message) + 1;

			ClientDatagram* outgoingDG = (ClientDatagram*)buffer;
			ssize_t fullDatagramSize = sizeof(ClientDatagram) + messageSize;


			// Set payload length
			outgoingDG->payload_length = messageSize + sizeof(ClientDatagram);
			outgoingDG->sequence_number = dgNum;
			strcpy(buffer + sizeof(ClientDatagram), message);

			if(isDebugMode)
			{
				cout << "Datagram created!";
				cout << " sn: " 	<< outgoingDG->sequence_number  << " {" 	<< sizeof(outgoingDG->sequence_number)	<< " bytes}";
				cout << " pl: " 	<< outgoingDG->payload_length	<< " {" 	<< sizeof(outgoingDG->payload_length) 	<< " bytes}";
				cout << " m:  " 	<< message					 	<< " {" 	<< messageSize							<< " bytes}";
				cout << " sz: " 	<< fullDatagramSize 			<< " bytes" << endl;
			}

			// Prepare ByteOrdering of Datagram for network
			outgoingDG->payload_length = htons(outgoingDG->payload_length);
			outgoingDG->sequence_number = htonl(outgoingDG->sequence_number);

			/*
				SEND THE DATAGRAM
			*/
			if(microsecDelay > 0) usleep(microsecDelay); // Delay before the next outgoing datagram if we want to

			if(isDebugMode) cout << "Attempting to send payload..." << endl;
			ssize_t bytesSent = sendto(udpSocketNumber, (void*)buffer, fullDatagramSize, 0, (struct sockaddr*) &serverSockAddr, sizeof(serverSockAddr));
			// Check if there was an error with sending
			if(bytesSent != fullDatagramSize)
			{
				cerr << "Sent: " << bytesSent << " expected to send: " << fullDatagramSize << endl;
				perror("sendto()");
				throw MESSAGE_SEND_ERROR_RV;
			}

			/*
				CHECK FOR RECIEVING
			*/
			bytesReceived = recvfrom(udpSocketNumber, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &serverSockAddr, &incomingSocketLength);
			if(errno == EAGAIN || errno == EWOULDBLOCK) continue; // These flags are set when there was nothing to read
			if(bytesReceived > 0)
			{
				// We recieved something
				ServerDatagram incomingDG = *((ServerDatagram*)buffer);
				incomingDG.datagram_length = ntohs(incomingDG.datagram_length);
				incomingDG.sequence_number = ntohl(incomingDG.sequence_number);
				cout << "Recieved datagram from server: ";
				cout << "     seq: " << incomingDG.sequence_number << " len: " << incomingDG.datagram_length << endl;
			}
			else
			{
				perror("recvfrom()");
				throw MESSAGE_RECV_ERROR_RV;
			}
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

	return retval;
}

const ClientDatagram BuildDatagram(char* buffer, uint32_t seqNum, char* message)
{
	ClientDatagram cdg;
	memset(&cdg, 0, sizeof(ClientDatagram)); // Clear the struct values

	cdg.sequence_number = seqNum;
	cdg.payload_length = strlen(message);
	memcpy(buffer, &cdg, sizeof(ClientDatagram)); // Copy our datagram
	memcpy(buffer + sizeof(ClientDatagram), message, cdg.payload_length); // Copy our message

	if(strcmp(GetDatagramMessage(buffer), message) != 0)
	{
		cerr << "**** ERROR: Message improperly transfered to datagram |";
		cerr << "Message: {" << message << "} but transferred: {" << GetDatagramMessage(buffer) << "}" << endl;
		throw 99;
	}

	return cdg;
}

inline char* GetDatagramMessage(char* datagramBuffer)
{
	return (datagramBuffer + sizeof(ClientDatagram));
}


void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress, bool& debugFlag, uint32_t& msDelay)
{
	int c;
	while ((c = getopt(argc, argv, "hs:p:dy:")) != -1)
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
				// -y introduces a delay between sending datagrams
				msDelay = atoi(optarg);
				break;
			}
			case ('s'):
			{
				// -s allows you to override the default of 127.0.0.1.
				serverAddress = string(optarg);
				break;
			}
			case ('p'):
			{
				// -p allows you to override the default destination port of 39390.
				portNumber = atoi(optarg);
				break;
			}
			case ('d'):
			{
				debugFlag = true;
				break;
			}
			default:
			{
				cerr << "ERROR, Invalid option [" << c << "] with argument: [" << optarg << "]" << endl;
				throw INVALID_OPTION;
			}
		}
	}
}
//--
void DisplayMenu(char* argv[])
{
	cerr << argv[0] << "options:" << endl;
	cerr << "	-h displays help" << endl;
	cerr << "	-s server_address (required)" << endl;
	cerr << "	-p port_number (required)" << endl;
	cerr << "   -m message (required)" << endl;
}
