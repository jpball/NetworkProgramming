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

#define SOCKET_FAIL 10;
#define HOSTENTRY_FAIL 11;

void GetOpt(int argc, char* argv[]);
void DisplayHelpMenu();
string ObtainMessage();

int main(int argc, char* argv[])
{
    int retVal = 0;
    int socketFD; // The socket number we will be using
    string serverAddress = DEFAULT_ADDRESS; // The IP address of the server we are connecting to
    int serverPort = DEFAULT_PORT; // The server port we are connecting to

    hostent* serverHostEntry;
    sockaddr_in serverSockAddr;

    try
    {
        // Establish TCP connection
        #pragma region SetupConnection

        socketFD = socket(PF_INET, SOCK_STREAM, 0);
        if(socketFD < 0)
        {
            perror("ERROR | socket():");
            throw SOCKET_FAIL;
        }

        serverHostEntry = gethostbyname(serverAddress.c_str());
        if(serverHostEntry == nullptr)
        {
            close(socketFD);
            cerr << "ERROR, no such host: " << serverAddress << endl;
            throw HOSTENTRY_FAIL;
        }

        // Clean up our connction details struct
        memset(&serverSockAddr, 0, sizeof(serverSockAddr));
        // We wanna use internet protocol
        serverSockAddr.sin_family = AF_INET;
        // Copy over some connection details from a connection entry to our detail struct
        memmove(&serverSockAddr.sin_addr.s_addr, serverHostEntry->h_addr, serverHostEntry->h_length);
        // Store the server port in our connection detail struct
        serverSockAddr.sin_port = htons(serverPort);

        if(connect(socketFD, (const sockaddr*)&serverSockAddr, socklen_t(sizeof(serverSockAddr))) == -1)
        {
            perror("ERROR, connect()");
            throw 99;
        }

        #pragma endregion

        #pragma region UserInput

        // Loop to gather message (OVER will end this loop)
        ssize_t bytesSent;
        while(true)
        {
            string fullOutgoingMessage = ObtainMessage();
            cout << "FULL MESSAGE: ";
            cout << fullOutgoingMessage << endl;

            bytesSent = send(socketFD, fullOutgoingMessage.c_str(), fullOutgoingMessage.size() + 1, 0);
            if(bytesSent != -1)
            {
                cout << "Success" << endl;
            }
            else
            {
                cout << "Fail" << endl;
            }

        }


        #pragma endregion
    }
    catch(int value)
    {
    }

    if(socketFD > 0)
    {
        close(socketFD);
    }

    cout << "Client exiting normally..." << endl;
    return retVal;
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
// Will return once a message featuring "OVER" is entered
string ObtainMessage()
{
    string fullOutgoingMessage;
    string userInputBuffer;

    while(true)
    {
        cout << "> "; // Print out prompt indicator
        getline(cin, userInputBuffer); // Get user input up until newline

        if(userInputBuffer.find("QUIT") != string::npos)
        {
            // We found 'QUIT' in the newly entered string
            // So let's quit out of the loop
            cout << "Quitting..." << endl;
            break;
        }

        size_t overIndex = userInputBuffer.find("OVER");
        if(overIndex != string::npos)
        {
            // Our inputting message contains the word OVER
            // And we only want up to that word
            // Note, we want to send the word "OVER" too, so add 4 to the offset
            fullOutgoingMessage.append(userInputBuffer.substr(0, overIndex + 4));
            break;
        }
        else
        {
            // Add this newly entered string to our overarching message
            fullOutgoingMessage.append(userInputBuffer);
        }
    }
    return fullOutgoingMessage;
}
