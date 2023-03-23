#include <iostream>
#include <getopt.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

#define HELP_OPT_RV 20
#define SOCKET_ERROR_RV 21
#define BIND_ERROR_RV 22
#define LISTEN_ERROR_RV 23
#define ACCEPT_ERROR_RV 24

#define DEFAULT_PORT 9838
#define DEFAULT_ADDRESS "127.0.0.1"
#define LISTEN_BACKLOG 5
#define DATA_BUFFER_SIZE 80


void GetOpt(int argc, char* argv[], int& portNum);
void PrintMenu();

int CreateListeningSocket();
void BindListeningSocket(const int& socketFD, const sockaddr_in& serverSockAddr);
void SetSocketToListenForConnections(const int& socketFD, const int backlogSize);
int GetNewConnection(const int& listenSocketFD, sockaddr_in& clientSockAddr);
void SetSocketOptions(const int& socketFD);
bool AddMessageToStrBuilder(string messageToAdd, stringstream& strBuilder);

int main(int argc, char* argv[])
{
    int retVal = 0;
    int incomingConnectionSocket_fd = -1; // The socket number we will be using to listen for connections
    int connectedSocket_fd = -1; // The socket that will be used as the connection and data transfer

    string serverAddress = DEFAULT_ADDRESS; // The IP address of the server we are connecting to
    int serverPort = DEFAULT_PORT; // The server port we are connecting to

    sockaddr_in serverSockAddr;
    sockaddr_in clientSockAddr;
    memset((void*)&serverSockAddr, 0, sizeof(serverSockAddr));

    try
    {
        GetOpt(argc, argv, serverPort);
        
        // Customize our server address info
        serverSockAddr.sin_family = AF_INET;
        serverSockAddr.sin_addr.s_addr = INADDR_ANY;
        serverSockAddr.sin_port = htons(serverPort);

        incomingConnectionSocket_fd = CreateListeningSocket();
        SetSocketOptions(incomingConnectionSocket_fd);
        BindListeningSocket(incomingConnectionSocket_fd, serverSockAddr);
        SetSocketToListenForConnections(incomingConnectionSocket_fd, LISTEN_BACKLOG);

        char dataBuffer[DATA_BUFFER_SIZE];
        
        while(true) // Outer client loop (for each client)
        {
            memset((void*)&clientSockAddr, 0, sizeof(clientSockAddr)); // Reset our client addr struct

            connectedSocket_fd = GetNewConnection(incomingConnectionSocket_fd, clientSockAddr);
            cout << "Recieved connection from " << inet_ntoa(clientSockAddr.sin_addr) << " from port " << clientSockAddr.sin_port << endl;

            stringstream fullMessageStrBuilder;
            bool readyToSendBack = false;

            while(true) // Inner Loop until client disconnected
            {
                memset(&dataBuffer, 0, DATA_BUFFER_SIZE); // Reset data buffer
                
                ssize_t br = recv(connectedSocket_fd, (void*)dataBuffer, DATA_BUFFER_SIZE, 0);
                if(br > 0)
                {
                    // No issues
                    readyToSendBack = AddMessageToStrBuilder(string(dataBuffer), fullMessageStrBuilder);
                    // Print out what the server has stored so far
                    cout << "Server Recieved: " << fullMessageStrBuilder.str() << endl;
                }
                else
                {
                    // Some issue with the recv
                    if(br == 0)
                    {
                        // Client disconnected
                        cout << "Client closed its connection" << endl;
                        close(connectedSocket_fd);
                        connectedSocket_fd = -1;
                        break;
                    }
                    else
                    {
                        perror("ERROR, recv()");
                    }
                }

                if(readyToSendBack)
                {
                    /*
                        Get the details of our outgoing message

                        Ex.
                        Line Length: 15 OVER
                    */ 
                    size_t sizeOfBuiltMessage = fullMessageStrBuilder.str().size();
                    string fullMessageToSend = "Line Length: " + to_string(sizeOfBuiltMessage) + " OVER";
                    ssize_t expectedBytesToSend = fullMessageToSend.size() + 1; // Number of bytes we are to send, including null terminator

                    cout << "Server sending: " << fullMessageToSend << endl;

                    // Send our line length message to the client
                    ssize_t sentBytes = send(connectedSocket_fd, fullMessageToSend.c_str(), fullMessageToSend.size() + 1, 0);
                    if(sentBytes != expectedBytesToSend)
                    {
                        // Bytes sent doesn't match expected count
                        perror("ERROR, send()");
                    }

                    fullMessageStrBuilder.str(""); // Clear the string builder
                    readyToSendBack = false; // Reset our sending flag
                }
            }
        }
    }
    catch(int eval)
    {
        retVal = eval;
    }

    if(incomingConnectionSocket_fd > 0)
    {
        close(incomingConnectionSocket_fd);
    }

    // Close our connected socket if it is open
    if(connectedSocket_fd > 0)
    {
        close(connectedSocket_fd);
    }


    cerr << "Returning with code: " << retVal << endl;
    return retVal;
}
//--
/*
    Parse the given message, and add it to our accumualting string builder
    THe code word 'OVER' will indicate the message is finished being recv, and needs to be sent

    Returns a bool indicating if the designated ending word was given, indicating if we are ready to send back
*/
bool AddMessageToStrBuilder(string messageToAdd, stringstream& strBuilder)
{
    // No issues
    bool doesMessageContainOVER = false;

    size_t overWordIndex;
    if((overWordIndex = messageToAdd.find("OVER")) != string::npos)
    {
        // Found the word OVER
        string subStr = messageToAdd.substr(0, overWordIndex);
        if(subStr.size() > 0) strBuilder << subStr; // There is substance to send
        doesMessageContainOVER = true;
    }
    else
    {
        // Recv string does not contain OVER
        strBuilder << messageToAdd;
    }

    return doesMessageContainOVER;
}
//--
/* 
Enable certain settings for the given socket
 - SO_REUSEADDR -> Reuse ports for other sockets
*/
void SetSocketOptions(const int& socketFD)
{
    int optval(1); // true (1)
    if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("ERROR, setsockopt(SO_REUSEADDR)");
    }
}
//--
/*
    Will accept an incoming connection request via the listening socket
    And will return the corresponding socket
*/
int GetNewConnection(const int& listenSocketFD, sockaddr_in& clientSockAddr)
{
    socklen_t clientAddrSize = socklen_t(sizeof(clientSockAddr));
    int newClientSocket = accept(listenSocketFD, (sockaddr*)&clientSockAddr, &clientAddrSize);
    if(newClientSocket == -1)
    {
        perror("ERROR, accept()");
        throw ACCEPT_ERROR_RV;
    }
    return newClientSocket;
}
//--
/*
    Create our initial socket to listen for incoming connections
*/
int CreateListeningSocket()
{
    int retSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(retSocket < 0)
    {
        perror("ERROR, socket(): ");
        return SOCKET_ERROR_RV;
    }
    return retSocket;
}
//--
/*
    Bind our listening socket to use our server connection info
*/
void BindListeningSocket(const int& socketFD, const sockaddr_in& serverSockAddr)
{
    if(bind(socketFD, ((const sockaddr*)&serverSockAddr), socklen_t(sizeof(serverSockAddr))) < 0)
    {
        perror("ERROR, bind()");
        throw BIND_ERROR_RV;
    }
}
//--
/*
    Set the given socket to listen for backlogSize number of incoming connections
*/
void SetSocketToListenForConnections(const int& socketFD, const int backlogSize)
{
    if(listen(socketFD, LISTEN_BACKLOG) == -1)
    {
        perror("ERROR, listen()");
        throw LISTEN_ERROR_RV;
    }
}
//--
/*
    Parse the command line options, potentially changing our port number
*/
void GetOpt(int argc, char* argv[],  int& portNum)
{
    int c;
    while((c = getopt(argc, argv, "hp:")) != -1)
    {
        switch(c)
        {
            default:
            case('h'):
            {
                PrintMenu();
                throw HELP_OPT_RV;
            }
            case('p'):
            {
                portNum = atoi(optarg);
                break;
            }
        }
    }
}
//--
/*
    Print our usage menu to the standard error
*/
void PrintMenu()
{
    cerr << "Usage: " << endl;
    cerr << "-h                prints this text" << endl;
    cerr << "-p port    overrides default port of 9838" << endl;
}
//--
