#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <netinet/in.h>
#include <getopt.h>
#include <arpa/inet.h>


#define LISTEN_BACKLOG 10
#define DEFAULT_SERVER_ADDR "127.0.0.1"
#define DEFAULT_SERVER_PORT 5077

#define USAGE_MENU_ERV 15
#define SOCKET_ERV 20
#define BIND_ERV 21
#define LISTEN_ERV 22
#define ACCEPT_ERV 30

using namespace std;

int EstablishListeningSocket(const sockaddr_in& serverSockAddr);
bool GetOpt(int argc, char* argv[], int& portNum);
int GetNewConnection(const int& listenSocketFD, sockaddr_in& clientSockAddr);


int main(int argc, char* argv[])
{
    int retVal = 0;
    string serverAddress = DEFAULT_SERVER_ADDR;
    int serverPort = DEFAULT_SERVER_PORT;

    sockaddr_in serverSockAddr;
    memset((void*)&serverSockAddr, 0, sizeof(serverSockAddr));

    int listeningSocketFD = -1;

    try
    {
        if(!GetOpt(argc, argv, serverPort))
        {
            cout << "Printing usage menu...FIXME" << endl;
            throw USAGE_MENU_ERV;
        }

        serverSockAddr.sin_family = AF_INET;
        serverSockAddr.sin_addr.s_addr = INADDR_ANY;
        serverSockAddr.sin_port = htons(serverPort);

        listeningSocketFD = EstablishListeningSocket(serverSockAddr);


        int newClient = -1;
        sockaddr_in clientSockAddr;
        memset((void*)&clientSockAddr, 0, sizeof(clientSockAddr));

        // Note: Accept() will hang until we get a connection
        newClient = GetNewConnection(listeningSocketFD, clientSockAddr);

        cout << "Client " << newClient << " connected from " << inet_ntoa(clientSockAddr.sin_addr) << " from port " << clientSockAddr.sin_port << endl;
    



    }
    catch(int eval)
    {
        retVal = eval;
    }
    
    
    return retVal;
}
//--
#pragma region ConnectionSetupFunctions
//--
int GetNewConnection(const int& listenSocketFD, sockaddr_in& clientSockAddr)
{
    socklen_t clientAddrSize = socklen_t(sizeof(clientSockAddr));
    int newClientSocket = accept(listenSocketFD, (sockaddr*)&clientSockAddr, &clientAddrSize);
    if(newClientSocket < 0)
    {
        perror("ERROR, accept()");
        throw ACCEPT_ERV;
    }

    return newClientSocket;
}
//--
/*
This function will create and link the socket that will listen
for incoming connection requests


RETURNS the listening socket file descriptor
*/
int EstablishListeningSocket(const sockaddr_in& serverSockAddr)
{
    int socketVal = socket(AF_INET, SOCK_STREAM, 0);
    if(socketVal < 0)
    {
        perror("ERROR, socket()");
        throw SOCKET_ERV;
    }
    
    // Bind our listening socket
    if(::bind(socketVal, ((const sockaddr*)&serverSockAddr), socklen_t(sizeof(serverSockAddr))) < 0)
    {
        if(socketVal > 0) close(socketVal);
        perror("ERROR, bind()");
        throw BIND_ERV;
    }

    // Set our socket to listen
    if(listen(socketVal, LISTEN_BACKLOG) == -1)
    {
        if(socketVal > 0) close(socketVal);
        perror("ERROR, listen()");
        throw LISTEN_ERV;
    }

    return socketVal;
}
//--
//--
#pragma endregion
//--



#pragma region UtilityFunctions
//--
/*
Parse the command land options supplied

-h will print the usage menu
-p will override the port number

RETURNS true if the options are valid, false if we need to print the usage menu and stop
*/
bool GetOpt(int argc, char* argv[], int& portNum)
{
    int c;
    while((c = getopt(argc, argv, "hp:")) != -1)
    {
        switch(c)
        {
            default:
            case('h'):
            {
                // -h or any abnormal option will cause GetOpt to 'fail'
                // Printing out the usage menu
                return false;
            }
            case('p'):
            {
                portNum = atoi(optarg);
                break;
            }
        }
    }
    return true;
}
//--
#pragma endregion

