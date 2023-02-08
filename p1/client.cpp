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

void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress);
void DisplayMenu(char* argv[]);

#define INVALID_OPTION 9;
#define HELP_MENU_RV 10;
#define SOCKET_ERROR_RV 11;
#define SERVER_CONNECTION_ERROR_RV 12;
#define MESSAGE_SEND_ERROR_RV 13;

int main(int argc, char * argv[])
{
	int retval = 0;
	const size_t BUFFER_SIZE = 1024;
	int udpSocketNumber;

	int serverPort = PORT_NUMBER;
	string serverAddress = SERVER_IP;

	struct sockaddr_in serverSockAddr;
	struct hostent* serverHostEnt;

	try
	{
		GetOptions(argc, argv, serverPort, serverAddress);

		// Create our outgoing data socket
		udpSocketNumber = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpSocketNumber < 0)
		{
			perror("ERROR opening socket");
			throw SOCKET_ERROR_RV;
		}

		// Get our server's connection info
		serverHostEnt = gethostbyname(serverAddress.c_str());
		if(serverHostEnt == nullptr)
		{
			close(udpSocketNumber);
			cerr << "ERROR, no such host: " << serverAddress << endl;
			throw SERVER_CONNECTION_ERROR_RV;
		}

		// Configure our connection settings
		memset(&serverSockAddr, 0, sizeof(serverSockAddr));
		serverSockAddr.sin_family = AF_INET;
		memmove(&serverSockAddr.sin_addr.s_addr, serverHostEnt->h_addr, serverHostEnt->h_length);
		serverSockAddr.sin_port = htons(serverPort);

		// Send our message to the server
		char buffer[BUFFER_SIZE];
		string message = "Hello";

		ssize_t messageLength = message.size() + 1 + sizeof(ClientDatagram);

		ClientDatagram clientDG;
		clientDG.sequence_number = 28;
		clientDG.payload_length = message.size() + 1;
		memcpy(buffer, &clientDG, sizeof(ClientDatagram)); // Copy our datagram
		memcpy(buffer + sizeof(ClientDatagram), message.c_str(), clientDG.payload_length); // Copy our message

		cout << "Created client: " << clientDG.sequence_number << " and " << clientDG.payload_length;
		cout << "  with message " << message << endl;

		ssize_t bytes_sent = sendto(udpSocketNumber, (void*)buffer, clientDG.payload_length + sizeof(clientDG), 0, (struct sockaddr*) &serverSockAddr, sizeof(serverSockAddr));
		if(bytes_sent != messageLength)
		{
			cerr << "Sent: " << bytes_sent << " expected to send: " << messageLength << endl;
			perror("sendto()");
			throw MESSAGE_SEND_ERROR_RV;
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


void GetOptions(int argc, char* argv[], int& portNumber, string& serverAddress)
{
	int c;
	while ((c = getopt(argc, argv, "hs:p:m:")) != -1)
	{
		switch (c) {

			case ('h'):
			{
				// -h prints help information and exits.
				DisplayMenu(argv);
				throw HELP_MENU_RV;
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
