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
#define DATA_BUFFER_SIZE 2048


void GetOpt(int argc, char* argv[], int& portNum);
void PrintMenu();

int CreateListeningSocket();
void BindListeningSocket(const int& socketFD, const sockaddr_in& serverSockAddr);
void SetSocketToListenForConnections(const int& socketFD, const int backlogSize);
int GetNewConnection(const int& listenSocketFD, sockaddr_in& clientSockAddr);
void SetSocketOptions(const int& socketFD);

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
        
        while(true) // Run for each client
        {
            memset((void*)&clientSockAddr, 0, sizeof(clientSockAddr)); // Reset our client addr struct

            connectedSocket_fd = GetNewConnection(incomingConnectionSocket_fd, clientSockAddr);
            cout << "Recieved connection from " << inet_ntoa(clientSockAddr.sin_addr) << " from port " << clientSockAddr.sin_port << endl;

            stringstream strBuilder;
            bool readyToSendBack = false;

            while(true) // Loop until client disconnected
            {
                memset(&dataBuffer, 0, DATA_BUFFER_SIZE); // Reset data buffer
                

                ssize_t br = recv(connectedSocket_fd, (void*)dataBuffer, DATA_BUFFER_SIZE, 0);
                if(br > 0)
                {
                    // No issues
                    string recvStr = string(dataBuffer);
                    size_t overWordIndex;
                    if((overWordIndex = recvStr.find("OVER")) != string::npos)
                    {
                        // Found the word OVER
                        string subStr = recvStr.substr(0, overWordIndex);
                        if(subStr.size() > 0) strBuilder << subStr;
                        readyToSendBack = true;
                    }
                    else
                    {
                        // Recv string does not contain OVER
                        strBuilder << recvStr;
                    }
                    cout << "Server Recieved: " << strBuilder.str() << endl;
                }
                else
                {
                    // Some issue with the recv
                    if(br == 0)
                    {
                        // Client disconnected
                        cout << "Client closed its connection" << endl;
                        break;
                    }
                    else
                    {
                        perror("ERROR, recv()");
                    }
                }

                if(readyToSendBack)
                {
                    size_t sizeOfBuiltMessage = strBuilder.str().size();
                    string fullMessageToSend = "Line Length: " + to_string(sizeOfBuiltMessage) + " OVER";
                    ssize_t expectedBytesToSend = fullMessageToSend.size() + 1;

                    cout << "Server sending: " << fullMessageToSend << endl;

                    ssize_t sentBytes = send(connectedSocket_fd, fullMessageToSend.c_str(), fullMessageToSend.size() + 1, 0);
                    if(sentBytes != expectedBytesToSend)
                    {
                        perror("ERROR, send()");
                    }
                    
                    strBuilder.str(""); // Clear the string builder
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

    if(connectedSocket_fd > 0)
    {
        close(connectedSocket_fd);
    }


    cout << "Returning..." << retVal << endl;
    return retVal;
}
//--
void SetSocketOptions(const int& socketFD)
{
    int optval(1); // true (1)
    if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("ERROR, setsockopt(SO_REUSEADDR)");
    }
}
//--
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
void BindListeningSocket(const int& socketFD, const sockaddr_in& serverSockAddr)
{
    if(bind(socketFD, ((const sockaddr*)&serverSockAddr), socklen_t(sizeof(serverSockAddr))) < 0)
    {
        perror("ERROR, bind()");
        throw BIND_ERROR_RV;
    }
}
//--
void SetSocketToListenForConnections(const int& socketFD, const int backlogSize)
{
    if(listen(socketFD, LISTEN_BACKLOG) == -1)
    {
        perror("ERROR, listen()");
        throw LISTEN_ERROR_RV;
    }
}
//--
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
void PrintMenu()
{
    cerr << "Usage: " << endl;
    cerr << "-h                prints this text" << endl;
    cerr << "-p port    overrides default port of 9838" << endl;
}
//--
