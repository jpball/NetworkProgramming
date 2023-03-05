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
#include <fcntl.h>

using namespace std;

#define DEFAULT_PORT 9838
#define DEFAULT_ADDRESS "127.0.0.1"
#define RECV_BUFFER_SIZE 2048

#define SOCKET_FAIL 10
#define HOSTENTRY_FAIL 11
#define CONNECT_FAIL 12
#define SETSOCKOPT_FAIL 13

void GetOpt(int argc, char* argv[]);
void DisplayHelpMenu();
int EstablishConnection(const string& serverAddress, const int& serverPort);

int main(int argc, char* argv[])
{
    int retVal = 0;
    int socketFD; // The socket number we will be using
    string serverAddress = DEFAULT_ADDRESS; // The IP address of the server we are connecting to
    int serverPort = DEFAULT_PORT; // The server port we are connecting to

    try
    {
        // Establish TCP connection
        socketFD = EstablishConnection(serverAddress, serverPort);

        // Loop to gather message (OVER will end this loop)
        ssize_t bytesSent;
        ssize_t bytesRecv;
        string message;

        char recvBuffer[RECV_BUFFER_SIZE];
        memset(&recvBuffer, 0, RECV_BUFFER_SIZE);
        bool messageOver = false;

        while(true)
        {
            messageOver = false;
            while(!messageOver)
            {
                cout << "> "; // Print out prompt indicator
                getline(cin, message); // Get user input up until newline

                size_t overIndex = message.find("OVER");
                if(overIndex != string::npos)
                {
                    // Our inputed message contains the word OVER
                    // And we only want up to that word
                    // Note, we want to send the word "OVER" too, so add 4 to the offset
                    message.erase(message.begin() + overIndex+4, message.end());
                    messageOver = true;
                }

                bytesSent = send(socketFD, message.c_str(), message.size() + 1, 0);
                if(bytesSent != message.size() + 1)
                {
                    // Abnormal number of bytes sent
                    perror("ERROR, send()");
                }
            }



            // Bytes sent is normal
            // Now we want to check to potentially receive from the server
            bytesRecv = recv(socketFD, (void*)&recvBuffer, RECV_BUFFER_SIZE, 0);
            if(bytesRecv <= 0)
            {
                switch(bytesRecv)
                {
                    case(0):
                    {
                        cout << "Server has closed..." << endl;
                        break;
                    }
                    case(-1):
                    default:
                    {
                        perror("ERROR, recv()");
                        break;
                    }
                }
            }
            else
            {
                cout << "Recieved from server: " << recvBuffer << endl;
                // MUST BE LAST STEP
                memset(&recvBuffer, 0, RECV_BUFFER_SIZE); // Reset our buffer
            }
        }

    }
    catch(int value)
    {
        retVal = value;
    }

    if(socketFD > 0)
    {
        close(socketFD);
    }

    cout << "Client exiting normally..." << endl;
    return retVal;
}
//--
// Returns the connect socket number
int EstablishConnection(const string& serverAddress, const int& serverPort)
{
    // Attempt to create our socket
    int socketFD = socket(PF_INET, SOCK_STREAM, 0);
    if(socketFD < 0)
    {
        perror("ERROR | socket():");
        throw SOCKET_FAIL;
    }

    // Enable port reusal
    int optval = 1;
    if(setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        close(socketFD); // Close on fail because main doesn't know the socket yet
        perror("ERROR, setsockopt():");
        throw SETSOCKOPT_FAIL;
    }

    // Obtain the connection details for our seever
    hostent* serverHostEntry = gethostbyname(serverAddress.c_str());
    if(serverHostEntry == nullptr)
    {
        close(socketFD); // Close on fail because main doesn't know the socket yet
        cerr << "ERROR, no such host: " << serverAddress << endl;
        throw HOSTENTRY_FAIL;
    }

    sockaddr_in serverSockAddrInfo; // Will store the details of our server
    // Clean up our connction details struct
    memset(&serverSockAddrInfo, 0, sizeof(serverSockAddrInfo));
    // We wanna use internet protocol
    serverSockAddrInfo.sin_family = AF_INET;
    // Copy over some connection details from a connection entry to our detail struct
    memmove(&serverSockAddrInfo.sin_addr.s_addr, serverHostEntry->h_addr, serverHostEntry->h_length);
    // Store the server port in our connection detail struct
    serverSockAddrInfo.sin_port = htons(serverPort);

    if(connect(socketFD, (const sockaddr*)&serverSockAddrInfo, socklen_t(sizeof(serverSockAddrInfo))) == -1)
    {
        perror("ERROR, connect()");
        close(socketFD); // Close on fail because main doesn't know the socket yet
        throw CONNECT_FAIL;
    }

    return socketFD;
}

//--
/*
Reads and saves all provided command line options
REQUIRED OPTIONS
- `h` prints help text to **stderr / cerr** and exits with return code 0.
- `p` overrides the default destination port.
- `d` overrides the default destination address.
*/
void GetOpt(int argc, char* argv[])
{
    int c;

    while ((c = getopt(argc, argv, "")) != -1)
    {
        switch(c)
        {
            case('h'):
            {
                break;
            }
            case('p'):
            {
                break;
            }
            case('d'):
            {
                break;
            }
            default:
                break;
        }
    }
}
//--
/*
Prints help menu, featuring command line options and their descriptions
*/
void DisplayHelpMenu()
{
    cout << "Usage:" << endl;
    cout << "-h       prints this text" << endl;
    cout << "-d ip    overrides the default address of " << DEFAULT_ADDRESS << endl;
    cout << "-p port  overrides the default port of " << DEFAULT_PORT << endl;
}
//--
