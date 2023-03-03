#include <iostream>
#include <getopt.h>
#include <string>
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

#define DEFAULT_PORT 9838
#define DEFAULT_ADDRESS "127.0.0.1"
#define LISTEN_BACKLOG 5
#define DATA_BUFFER_SIZE 2048

int main(int argc, char* argv[])
{
    int retVal = 0;
    int incomingConnectionSocket_fd = -1; // The socket number we will be using to listen for connections
    int connectedSocket_fd = -1; // The socket that will be used as the connection and data transfer

    string serverAddress = DEFAULT_ADDRESS; // The IP address of the server we are connecting to
    int serverPort = DEFAULT_PORT; // The server port we are connecting to

    sockaddr_in serverSockAddr;
    sockaddr_in clientSockAddr;

    memset((void*)&clientSockAddr, 0, sizeof(clientSockAddr));
    memset((void*)&serverSockAddr, 0, sizeof(serverSockAddr));

    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_addr.s_addr = INADDR_ANY;
    serverSockAddr.sin_port = htons(serverPort);

    try{
        incomingConnectionSocket_fd = socket(PF_INET, SOCK_STREAM, 0);
        if(incomingConnectionSocket_fd < 0)
        {
            perror("ERROR, socket(): ");
            return 99;
        }

        if(bind(incomingConnectionSocket_fd, ((const sockaddr*)&serverSockAddr), socklen_t(sizeof(serverSockAddr))) < 0)
        {
            perror("ERROR, bind()");
            throw 101;
        }

        if(listen(incomingConnectionSocket_fd, LISTEN_BACKLOG) == -1)
        {
            perror("ERROR, listen()");
            throw 102;
        }

        socklen_t clientAddrSize = socklen_t(sizeof(clientSockAddr));
        connectedSocket_fd = accept(incomingConnectionSocket_fd, (sockaddr*)&clientSockAddr, &clientAddrSize);

        if(connectedSocket_fd == -1)
        {
            perror("ERROR, accept()");
            throw 103;
        }

        char buffer[DATA_BUFFER_SIZE];
        while(true)
        {
            ssize_t br = recv(connectedSocket_fd, (void*)&buffer, DATA_BUFFER_SIZE, 0);

            if(br == 0) break;
            if(br == -1)
            {
                perror("recv()");
            }


            cout << "Recv: " << br << " bytes" << endl;
            cout << "Buffer: " << buffer << endl;
            cout << "---------------------------" << endl;

            memset(buffer, 0, 2048);
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
