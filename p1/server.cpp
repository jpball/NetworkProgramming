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
inline string ClientDatagramToString(ClientDatagram* p_cd);

#define HELP_MENU_RV 10;
#define SOCKET_ERROR_RV 11;
#define BIND_ERROR_RV 12;
#define UNKNOWN_OPTION_RV 21;

int main(int argc, char * argv[])
{
	int retval = 0; // 0 means success, otherwise some error based on a predefined macro
	const size_t BUFFER_SIZE = 2048; // Determine the number of bytes
	int udpSocketNumber; // Our socket file descriptor
	int serverPort = PORT_NUMBER; // The port to use for our incoming/outgoing connection
	bool isDebugMode = false;

	struct sockaddr_in serverSockAddr; // Hold the personal connection details
	struct sockaddr_in clientSockAddr; // Hold the outgoing connection details
	unsigned char buffer[BUFFER_SIZE]; // Act as our data construction buffer for the incoming bytes
	ssize_t bytesReceived; // Will hold the number of bytes we recieve from recvfrom()
	ssize_t bytesSent; // Will keep track of how many bytes are sent back to the client
	socklen_t clientSocketLength = sizeof(clientSockAddr);
	memset((void *) &clientSockAddr, 0, sizeof(clientSockAddr));

	try
	{
		GetOptions(argc, argv, serverPort, isDebugMode);

		cout << "Server will attempt to bind to port: " << serverPort << endl;

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

		cout << "Network structs initialized and port is bound." << endl;

		// Since it's a server, we never want to really end
		while (true)
		{
			memset(buffer, 0, BUFFER_SIZE); // Wipe out any garbage/leftover values from our buffer

			bytesReceived = recvfrom(udpSocketNumber, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientSockAddr, &clientSocketLength);

			if (bytesReceived > 0)
			{
				if(isDebugMode)
				{
					cout << bytesReceived << " bytes received from: ";
					cout << inet_ntoa(clientSockAddr.sin_addr) << endl;
				}

				// Build a client datagram using the buffer
				ClientDatagram* incomingDG = (ClientDatagram*)buffer;

				// Interpret the values from network byte ordering
				incomingDG->payload_length = ntohs(incomingDG->payload_length);
				incomingDG->sequence_number = ntohl(incomingDG->sequence_number);

				if(isDebugMode)
				{
					cout << "Recieved datagram | " << ClientDatagramToString((ClientDatagram*)buffer) << endl;
				}

				// Now that we got our incoming data, let's use it and send back a response datagram
				// Ensure we convert to network byte ordering
				ServerDatagram outgoingDG;
				outgoingDG.sequence_number = htonl(incomingDG->sequence_number);
				outgoingDG.datagram_length = htons(bytesReceived);

				bytesSent = sendto(udpSocketNumber, &outgoingDG, sizeof(outgoingDG), 0, (sockaddr*)&clientSockAddr, sizeof(clientSockAddr));
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
			else if (bytesReceived == 0)
			{
				cout << "Received zero bytes - I don't think this can happen.\n";
			}
			else
			{
				// Recv failed...we don't want to end just yet
				perror("recvfrom()");
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
inline string ClientDatagramToString(ClientDatagram* p_cd)
{
	return string("ClientDatagram | seqnum: " + to_string(p_cd->sequence_number) + " pl: " + to_string(p_cd->payload_length) + " m: " + ((char*)p_cd + sizeof(ClientDatagram)));
}
