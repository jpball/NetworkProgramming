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

void GetOptions(int argc, char* argv[], int& portNumber);
void DisplayMenu(char* argv[]);

#define HELP_MENU_RV 10;
#define SOCKET_ERROR_RV 11;
#define BIND_ERROR_RV 12;
#define INVALID_BYTES_SENT_RV 20;

int main(int argc, char * argv[])
{
	int retval = 0;
	const size_t BUFFER_SIZE = 1024;
	int udpSocketNumber;
	int serverPort = PORT_NUMBER;

	struct sockaddr_in serverSockAddr;
	struct sockaddr_in clientSockAddr;
	unsigned char buffer[BUFFER_SIZE];
	ssize_t bytesReceived;
	socklen_t l = sizeof(clientSockAddr);

	memset((void *) &clientSockAddr, 0, sizeof(clientSockAddr));

	try
	{
		GetOptions(argc, argv, serverPort);

		cout << "Server will attempt to bind to port: " << serverPort << endl;

		// Create our incoming socket
		udpSocketNumber = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpSocketNumber < 0)
		{
			perror("ERROR opening socket");
			throw SOCKET_ERROR_RV;
		}

		// Configure our connection details
		memset(&serverSockAddr, 0, sizeof(serverSockAddr));
		serverSockAddr.sin_family = AF_INET;
		serverSockAddr.sin_addr.s_addr = INADDR_ANY;
		serverSockAddr.sin_port = htons(serverPort);

		// Bind our socket to our connection details
		if (bind(udpSocketNumber, (const struct sockaddr *) &serverSockAddr, socklen_t(sizeof(serverSockAddr))) < 0)
		{
			close(udpSocketNumber);
			perror("ERROR on binding");
			throw BIND_ERROR_RV;
		}
		cout << "Network structs initialized and port is bound." << endl;

		while (true)
		{
			memset(buffer, 0, BUFFER_SIZE);
			bytesReceived = recvfrom(udpSocketNumber, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &clientSockAddr, &l);

			if (bytesReceived > 0)
			{
				cout << bytesReceived << " bytes received from: ";
				cout << inet_ntoa(clientSockAddr.sin_addr) << endl;

				ClientDatagram incomingDG = *((ClientDatagram*)buffer);
				char* message = (char*)buffer + sizeof(ClientDatagram);
				cout << "Recieved datagram: " << incomingDG.sequence_number << " seq num, " << incomingDG.payload_length << " payload long." << endl;
				cout << "Message: " << message << endl;


				ServerDatagram outgoingDG;
				outgoingDG.sequence_number = incomingDG.sequence_number;
				outgoingDG.datagram_length = bytesReceived;

				ssize_t bytesSent = sendto(udpSocketNumber, (void*)&outgoingDG, sizeof(outgoingDG), 0, (sockaddr*)&clientSockAddr, sizeof(clientSockAddr));
				if(bytesSent != sizeof(ServerDatagram))
				{
					cerr << "Sent: " << bytesSent << " expected to send: " << sizeof(ServerDatagram) << endl;
					perror("sendto()");
					throw INVALID_BYTES_SENT_RV;
				}
			}
			else if (bytesReceived == 0)
			{
				cout << "Received zero bytes - I don't think this can happen.\n";
			}
			else {
				perror("recvfrom()");
				throw 1;
			}
		}
	}
	catch (int rv)
	{
		retval = rv;
	}
	return retval;
}


void GetOptions(int argc, char* argv[], int& portNumber)
{
	int c;
	while ((c = getopt(argc, argv, "hp:vd")) != -1)
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
		}
	}
}
//--
void DisplayMenu(char* argv[])
{
	cerr << argv[0] << "options:" << endl;
	cerr << "   -h displays help" << endl;
	cerr << "   -p port_number" << endl;
}
