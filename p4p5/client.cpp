/*
    CSC 3700: Network Programming
    Project 4/5: Retro Chat - Client

    Jordan Ball
*/

#include <iostream>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <netinet/in.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <csignal>
#include <sstream>

using namespace std;

#define INPUTWIN_X 0
#define INPUTWIN_Y LINES - 3
#define CONVOWIN_X 0
#define CONVOWIN_Y 0
#define CONVO_WIN_BOTY INPUTWIN_Y - 2

// Function Prototypes
void SIGINTHandler(int signum);
void RemoveLastCharacter(stringstream& stream);
void InitializeScreen();
void UpdateConvoWindowWithNewMessage(string message);
inline void ResetInputCursor();
inline void DrawConvoWindowBox();
inline void DrawInputWindowBox();
void ResetInputWindow();

bool continueFlag;

WINDOW* fullScr;
WINDOW* inputWindow; // The window the user will use to input messages
WINDOW* convoWindow; // The window containing any recv'd messages and the message history

stringstream inputBuffer; // Stores the user input, eventually used to send

int main(int argc, char* argv[])
{
    signal(SIGINT, SIGINTHandler);
    continueFlag = true;

    /*
        Setup the GUI
    */
    fullScr = initscr();
    convoWindow = subwin(fullScr, LINES - 3, COLS, CONVOWIN_Y, CONVOWIN_X);
    inputWindow = subwin(fullScr, 3, COLS, INPUTWIN_Y, INPUTWIN_X);

    InitializeScreen(); // Sets up the window in the buffer, needs to be refreshed

    int inputResult;

    while(continueFlag)
    {
        // HANDLE USER INPUT
        inputResult = wgetch(inputWindow);
        if(inputResult == ERR) continue; // NonBlocking will have wgetch return ERR
        if(!continueFlag) break; // If sigint was indicated, we wanna exit

        if(inputResult == '\n') // Enter
        {
            ResetInputWindow();
            string word;
            inputBuffer >> word;
            UpdateConvoWindowWithNewMessage(word);
        }
        else if(inputResult == 127) // Backspace
        {
            if(getcurx(inputWindow) <= 1) continue; // We don't want to go off the window or onto the border
            RemoveLastCharacter(inputBuffer); // Removes the last character from the stream
            
            // Remove the character on the screen we are deleting
            mvwaddch(inputWindow, 1, getcurx(inputWindow) - 1, ' '); // Remove the character from the screen
            wmove(inputWindow, 1, getcurx(inputWindow) - 1); // Move the cursor back a space
            wrefresh(inputWindow); // Refresh the input window without the deleted character
        }
        else if(isprint(inputResult)) // Any letter/num/symbol
        {
            inputBuffer << char(inputResult); // Add our input to the buffer stream
            mvwaddch(inputWindow, 1, getcurx(inputWindow), inputResult); // add the inputted character to the screen
            wrefresh(inputWindow); // Refresh the window with the newly inputted character
        }
    }

    endwin();
    return 0;
}
//--
#pragma region NetworkFunctions

#pragma endregion
//--
#pragma region GUI_BACKEND
//--
/*
    This function will be called anytime SIGINT (^c) is pressed
*/
void SIGINTHandler(int signum)
{
    echo();
    nodelay(inputWindow, false);
    cbreak();
    curs_set(1);
    system("stty sane");
    continueFlag = false;
}
//--
/*
    This will remove the last character added to a string stream
    - Updates internal string and the input seeker
*/
void RemoveLastCharacter(stringstream& stream)
{
    stream.seekp(-1, std::ios_base::end); // Reset our buffer to start input before the last character
    string copy = stream.str();
    copy.pop_back();
    stream.str(copy);
}
//--
#pragma endregion
//--
#pragma region GUI_FRONTEND
//--
/*
    Will update the convo window with a new message at the bottom
    REFRESHES CONVO WINDOW
*/
void UpdateConvoWindowWithNewMessage(const string message)
{
    scroll(convoWindow);
    DrawConvoWindowBox();
    mvwaddstr(convoWindow, CONVO_WIN_BOTY, 1, message.c_str());
    wrefresh(convoWindow);
}
//--
/*
    Draw the basic framework of the Convo and Input windows
    Enables/Disables specific settings for the screen's behavior
    REFRESHES the screen
*/
void InitializeScreen()
{   
    noecho(); // We don't want user input to echo into the console
    nodelay(inputWindow, true); // We want non-blocking input fetching
    cbreak(); // We want character at a time input
    curs_set(1); // Disable cursor

    DrawConvoWindowBox(); // Draw the outline of the convo window
    scrollok(convoWindow, true);
    wsetscrreg(convoWindow, 1, CONVO_WIN_BOTY); // Sets the scroll regions of the convo window

    DrawInputWindowBox(); // Draw the outline of the input window
    ResetInputCursor();
    refresh();
}
//--
/*
    Will reset the cursor of the inputWindow to the starting position (1,1)
    DOES NOT REFRESH
*/
inline void ResetInputCursor()
{
    // Move the cursor to the input window
    wmove(inputWindow, 1, 1);
}
//--
/*
    Completely erases and redraws the input window
    REFRESHES the Input Window as well
*/
void ResetInputWindow()
{
    werase(inputWindow);
    DrawInputWindowBox();
    ResetInputCursor();
    wrefresh(inputWindow);
}
//--
/*
    Draws the outer box and label of the input window
    DOES NOT REFRESH
*/
inline void DrawInputWindowBox()
{
    // Set up the Input Window
    box(inputWindow, 0, 0); // Draw box outline
    mvwaddstr(inputWindow, CONVOWIN_Y, CONVOWIN_X + 2, "Text Entry"); // Add descriptive heading
}
//--
/*
    Draws the outer box and label of the Conversation Window
    DOES NOT REFRESH
*/
inline void DrawConvoWindowBox()
{
    box(convoWindow, 0,0); // Draw box outline
    mvwaddstr(convoWindow, CONVOWIN_Y, CONVOWIN_X + 2, "Conversation"); // Add descriptive heading
}
//--
#pragma endregion