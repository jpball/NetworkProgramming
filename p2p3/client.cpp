#include <iostream>
#include <getopt.h>
#include <string>

using namespace std;

void GetOpt(int argc, char* argv[]);
void DisplayHelpMenu();

int main(int argc, char* argv[])
{
    cout << "Hello world!" << endl;

    try
    {
        // Establish TCP connection
        // Loop to gather message (OVER will end this loop)
        string fullOutgoingMessage;
        string userInput;

        do
        {
            getline(cin, userInput);
            fullOutgoingMessage.append(userInput);
        }while(userInput.find("QUIT") == string::npos);

        cout << "FULL MESSAGE:" << endl;
        cout << fullOutgoingMessage << endl;

    }
    catch(int value)
    {
    }
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
    cout << ":: Options ::" << endl;
    cout << "    -h prints this menu" << endl;
    cout << "    -p overrides port number" << endl;
    cout << "    -d overrides destination address" << endl;
}
