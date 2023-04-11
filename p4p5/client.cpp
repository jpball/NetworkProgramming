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

// Functionality Prototypes
void SIGINTHandler(int signum);
void RemoveLastCharacter(stringstream& stream);
// GUI Function Prototypes
void InitializeScreen();
void UpdateConvoWindowWithNewMessage(string message);
inline void ResetInputCursor();
inline void DrawConvoWindowBox();
inline void DrawInputWindowBox();

bool continueFlag;

WINDOW* fullScr;
WINDOW* inputWindow;
WINDOW* convoWindow;

stringstream inputBuffer;

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
    refresh();

    int res;
    ResetInputCursor();

    while(continueFlag)
    {
        res = wgetch(inputWindow);
        if(res == ERR) continue;
        if(!continueFlag) break;

        if(res == '\n') // Enter
        {
            UpdateConvoWindowWithNewMessage(inputBuffer.str());
            wrefresh(convoWindow);
            RemoveLastCharacter(inputBuffer);
            DrawInputWindowBox();
            inputBuffer.str("");
            ResetInputCursor();

            wrefresh(inputWindow);
        }
        else if(res == 127) // Backspace
        {
            if(getcurx(inputWindow) <= 1) continue; // We don't want to go off the window or onto the border
            
            mvwaddstr(convoWindow, 5, 1, inputBuffer.str().c_str());
            
            mvwaddstr(convoWindow, 6, 1, inputBuffer.str().c_str());
            wrefresh(convoWindow);
            

            mvwaddch(inputWindow, 1, getcurx(inputWindow) - 1, ' ');
            wmove(inputWindow, 1, getcurx(inputWindow) - 1);
            wrefresh(inputWindow);
        }
        else if(isprint(res)) // Any letter/num/symbol
        {
            inputBuffer << char(res);
            mvwaddch(inputWindow, 1, getcurx(inputWindow), res);
            wrefresh(inputWindow);
        }
    }

    endwin();
    return 0;
}
//--
#pragma region Functionality_Functions
/*
    This function will be called anytime SIGINT (^c) is pressed
*/
void SIGINTHandler(int signum)
{
    echo();
    nodelay(inputWindow, false);
    cbreak();
    curs_set(1);
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
#pragma region GUI_Functions
//--
/*
    Will update the convo window with a new message at the bottom
*/
void UpdateConvoWindowWithNewMessage(const string message)
{
    scroll(convoWindow);
    DrawConvoWindowBox();
    mvwaddstr(convoWindow, CONVO_WIN_BOTY, 1, message.c_str());
}
//--
/*
    Draw the basic framework of the Convo and Input windows
*/
void InitializeScreen()
{   
    noecho(); // We don't want user input to echo into the console
    nodelay(inputWindow, true); // We want non-blocking input fetching
    cbreak(); // We want character at a time input
    curs_set(1); // Disable cursor


    DrawConvoWindowBox();

    DrawInputWindowBox();

}
//--
/*
    Will reset the cursor of the inputWindow to the starting position (1,1)
*/
inline void ResetInputCursor()
{
    // Move the cursor to the input window
    wmove(inputWindow, 1, 1);
}
//--
inline void DrawInputWindowBox()
{
    // Set up the Input Window
    box(inputWindow, 0, 0); // Draw box outline
    mvwaddstr(inputWindow, CONVOWIN_Y, CONVOWIN_X + 2, "Text Entry"); // Add descriptive heading
}
//--
inline void DrawConvoWindowBox()
{
    box(convoWindow, 0,0); // Draw box outline
    mvwaddstr(convoWindow, CONVOWIN_Y, CONVOWIN_X + 2, "Conversation"); // Add descriptive heading
}
//--
#pragma endregion