/*** Includes ***/
#include <stdio.h>
#include <windows.h>
#include <ctype.h>
#include <unistd.h>

/*** Defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** Data ***/
HANDLE hStdin;
DWORD originalMode;
typedef struct
{
    int Rows;
    int Columns;
} WindowSize;


/*** Prototypes ***/
void EnableRawMode();
char EditorReadKey();
void DisableRawMode();
void EditorDrawRows();
void EditorRefreshScreen();
WindowSize* GetWindowSize();
void EditorProcessKeypress();
void Die( const char* message );

/*** Init ***/
int main()
{
    EnableRawMode(); // Disables certain terminal behaviours

    while (1)
    {
        EditorRefreshScreen(); // Refresh terminal window
        EditorDrawRows(); // Draw ~ for all the rows in the terminal window
        EditorProcessKeypress(); // Process each keypress, the user has registered
    }

    return 0;
}

/*** Terminal ***/
void EnableRawMode()
{
    hStdin = GetStdHandle( STD_INPUT_HANDLE );
    DWORD newMode;

    // Current console mode
    GetConsoleMode( hStdin , &originalMode );

    // At exit, disable raw mode
    atexit( DisableRawMode );

    // Copy the default console mode to modify that
    newMode = originalMode;

    // Turn off ECHO mode
    // Turn off Canonical mode
    // Turn off Ctrl-Z and Ctrl-C
    newMode &= ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT );
    // Turn off Ctrl-S and Ctrl-Q
    newMode &= ~( ENABLE_PROCESSED_OUTPUT );

    // Change the current mode with the new configuration
    if (!SetConsoleMode( hStdin , newMode )) Die( "Enable" );
}

void DisableRawMode()
{
    // Set the console to the default mode
    if (!SetConsoleMode( hStdin , originalMode )) Die( "Disable" );
}

void Die( const char* message )
{
    EditorRefreshScreen();

    perror( message );
    exit( 1 );
}

char EditorReadKey()
{
    int nread;
    char c;

    // Save all keypresses tp the char c, if not char is entered exit
    while (( nread = read( STDIN_FILENO , &c , 1 ) ) != 1)
    {
        if (nread != 1 && errno != EAGAIN) Die( "Read" );
    }

    return c;
}

WindowSize* GetWindowSize()
{
    // Handle to the console output
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );

    // Object to hold console details
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

    WindowSize* pWindow;

    if (GetConsoleScreenBufferInfo( hConsole , &consoleInfo ))
    {
        // Calculate terminal width and height
        pWindow->Columns = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
        pWindow->Rows = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;
    }

    return pWindow;
}

/*** Input ***/
void EditorProcessKeypress()
{
    char c = EditorReadKey();

    switch (c)
    {
        // If the Ctrl-Q is pressed, the editor will exit
        case CTRL_KEY( 'q' ):
            EditorRefreshScreen();

            exit( 0 );
            break;
    }
}

/*** Output ***/
void ResetCursor()
{
    // Positions the cursor to the top left
    write( STDOUT_FILENO , "\x1b[H" , 3 );
}

void EditorRefreshScreen()
{
    write( STDOUT_FILENO , "\x1b[2J" , 4 );
    ResetCursor();
}

void EditorDrawRows()
{
    WindowSize* pConsoleWindow = GetWindowSize();

    int i;
    for (i = 0; i < pConsoleWindow->Rows; i++)
    {
        write( STDOUT_FILENO , "~\r\n" , 3 );
    }
    ResetCursor();
}