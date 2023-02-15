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

void GetOpt(int argc, char* argv[]);
void DisplayHelpMenu();
void SendMessageToServer(const string message);

int main(int argc, char* argv[])
{
    int retVal = 0;
    int socketFD; // The socket number we will be using
    string serverAddress = DEFAULT_ADDRESS; // The IP address of the server we are connecting to
    int serverPort = DEFAULT_PORT; // The server port we are connecting to


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


        #pragma endregion

        #pragma region UserInput
        /*
        // Loop to gather message (OVER will end this loop)

        string fullOutgoingMessage;
        string userInput;


        while(true)
        {
            cout << "> "; // Print out prompt indicator
            getline(cin, userInput); // Get user input up until newline

            if(userInput.find("QUIT") != string::npos)
            {
                // We found 'QUIT' in the newly entered string
                // So let's quit out of the loop
                cout << "Quitting..." << endl;
                break;
            }

            size_t overIndex = userInput.find("OVER");
            if(overIndex != string::npos)
            {
                // Our inputting message contains the word OVER
                // And we only want up to that word
                // Note, we want to send the word "OVER" too, so add 4 to the offset
                fullOutgoingMessage.append(userInput.substr(0, overIndex + 4));

                // Send it to our server
                SendMessageToServer(fullOutgoingMessage);
            }

            // Add this newly entered string to our overarching message
            fullOutgoingMessage.append(userInput);

        }
        do
        {



            fullOutgoingMessage.append(userInput);


        }while(userInput.find("QUIT") == string::npos);

        cout << "FULL MESSAGE:" << endl;
        cout << fullOutgoingMessage << endl;

        */
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
    return 0;
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
void SendMessageToServer(const string message)
{
    cout << "Server probably recieved: |" << message << "|" << endl;
}
