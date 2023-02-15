#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#ifdef	 __linux__
#include <unistd.h>
#endif
#include <getopt.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>

#include "defaults.hpp"
#include "structure.hpp"

using namespace std;

void GetOptions(int argc, char* argv[], int& portNumber, bool& debugFlag);
void DisplayMenu(char* argv[]);
inline void PrintClientDatagramInfo(void* buffer);
inline void PrintServerDatagramInfo(ServerDatagram* buffer);

#define HELP_MENU_RV 10;
#define SOCKET_ERROR_RV 11;
#define BIND_ERROR_RV 12;
#define UNKNOWN_OPTION_RV 21;

int main(int argc, char * argv[])
{
	int retval = 0;
	int udpSocketNumber;
	int serverPort = PORT_NUMBER;
	bool isDebugMode = false;

	sockaddr_in serverSockAddr;
	sockaddr_in clientSockAddr;
	socklen_t socketAddrSize = sizeof(clientSockAddr);
	memset((void *) &clientSockAddr, 0, sizeof(clientSockAddr));

	// Build a client datagram using the buffer
	// Since we don't know the exact size, let's just have a buffer size
	const size_t BUFFER_SIZE = 1024;
	ssize_t bytesReceived;
	char* incomingBuffer = (char*)malloc(BUFFER_SIZE);
	ClientDatagram* incomingDG = (ClientDatagram*)incomingBuffer;

	// Let's create a blank Datagrams to make out outgoing construction faster
	ssize_t bytesSent = 0;
	ServerDatagram* outgoingDG = new ServerDatagram;

	try
	{
		GetOptions(argc, argv, serverPort, isDebugMode);

		cout << "Server will attempt to bind to port: " << serverPort << endl;

		#pragma region CONNECTION_SETUP

		// Create our incoming socket
		udpSocketNumber = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpSocketNumber < 0)
		{
			perror("ERROR opening socket");
			throw SOCKET_ERROR_RV;
		}

		// Configure our connection details
		memset(&serverSockAddr, 0, sizeof(sockaddr_in));
		serverSockAddr.sin_family = AF_INET; // Address family (ipv4 or ipv6, AF_INET allows both)
		serverSockAddr.sin_addr.s_addr = INADDR_ANY; // Any ip address
		serverSockAddr.sin_port = htons(serverPort); // The port we want to use

		// Bind our socket to our connection details
		if (bind(udpSocketNumber, (const struct sockaddr *) &serverSockAddr, socklen_t(sizeof(serverSockAddr))) < 0)
		{
			perror("ERROR on binding");
			throw BIND_ERROR_RV;
		}
		#pragma endregion

		cout << "Network structs initialized and port is bound." << endl;

		// Since it's a server, we never want to really end
		while (true)
		{
			memset(incomingBuffer, 0, BUFFER_SIZE); // Wipe out any garbage/leftover values from our buffer
			bytesReceived = recvfrom(udpSocketNumber, incomingBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientSockAddr, &socketAddrSize);

			if (bytesReceived > 0)
			{
				// Convert our data to the host byte order
				incomingDG->payload_length = ntohs(incomingDG->payload_length);
				incomingDG->sequence_number = ntohl(incomingDG->sequence_number);

				if(isDebugMode)
				{
					cout << bytesReceived << " bytes received from: ";
					cout << inet_ntoa(clientSockAddr.sin_addr) << endl;
					cout << "Recieved Datagram: ";
					PrintClientDatagramInfo(incomingBuffer);
				}

				// Now that we got our incoming data, let's use it and send back a response datagram
				// Using the received data, use it to found our outgoing Datagram
				outgoingDG->datagram_length = htons(bytesReceived);
				outgoingDG->sequence_number = htonl(incomingDG->sequence_number);

				bytesSent = sendto(udpSocketNumber, outgoingDG, sizeof(ServerDatagram), 0, (sockaddr*)&clientSockAddr, socketAddrSize);

				if(bytesSent != sizeof(ServerDatagram))
				{
					// We sent less than we wanted
					cerr << "ERROR: Sent " << bytesSent << " bytes, expected to send: " << sizeof(ServerDatagram) << endl;
					perror("sendto()");
				}
				else if(isDebugMode)
				{
					// We sent the proper amount of bytes
					cerr << "Sent " << bytesSent << " bytes!" << endl;
				}
			}
			else
			{
				if(bytesReceived == 0) cout << "Received zero bytes - I don't think this can happen." << endl;
				else
				{
					perror("ERROR | recvfrom():");
				}
			}
		}
	}
	catch (int rv)
	{
		retval = rv;
	}

	// Close our socket
	if(udpSocketNumber >= 0)
	{
		close(udpSocketNumber);
	}

	// Free up our outgoing buffer
	if(incomingBuffer != nullptr)
	{
		free(incomingBuffer);
		incomingBuffer = nullptr;
	}

	// Free up the heap's server datagram
	if(outgoingDG != nullptr)
	{
		delete outgoingDG;
		outgoingDG = nullptr;
	}

	return retval;
}

void GetOptions(int argc, char* argv[], int& portNumber, bool& debugFlag)
{
	int c;
	while ((c = getopt(argc, argv, "hp:d")) != -1)
	{
		switch (c) {

			case ('h'):
			{
				DisplayMenu(argv);
				throw HELP_MENU_RV;
			}
			case ('p'):
			{
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
				cerr << "Unknown option supplied: " << (char)c << endl;
				throw UNKNOWN_OPTION_RV;
			}
		}
	}
}
//--
void DisplayMenu(char* argv[])
{
	cout << argv[0] << " options:" << endl;
	cout << "   -h displays help" << endl;
	cout << "   -p port_number ..... defaults to " << PORT_NUMBER << endl;
	cout << "   -d enables debug mode" << endl;
}
//--
inline void PrintClientDatagramInfo(void* buffer)
{
	// The byte orders are not converted
	ClientDatagram* cd = (ClientDatagram*)buffer;
	cout << "ClientDatagram - seqnum: " << cd->sequence_number << " pl: " << cd->payload_length << " m: " << (char*)cd + sizeof(ClientDatagram) << endl;
}
//--
inline void PrintServerDatagramInfo(ServerDatagram* buffer)
{
	ServerDatagram* sd = (ServerDatagram*)buffer;
	cout << "ClientDatagram - seqnum: " << sd->sequence_number << " dl: " << sd->datagram_length << endl;
}
