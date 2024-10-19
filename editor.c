#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>
#include <conio.h>

/*** Defines ***/
#define EDITOR_VERSION "0.1.0"
#define CTRL_KEY(k) ((k) & 0x1f)
#define STR_BUFFER_INIT {NULL, 0}

/*** Data ***/
typedef struct
{
    int Rows;
    int Columns;
} WindowSize;

typedef struct
{
    char* b;
    int len;
} StrBuffer;

typedef struct
{
    int x;
    int y;
} Cursor;

HANDLE hStdin;
StrBuffer* sb;
Cursor cursor; // Update to direct structure
DWORD originalMode;

/*** Prototypes ***/
void EditorInit();
void EnableRawMode();
char EditorReadKey();
void DisableRawMode();
void EditorDrawRows();
void MoveCursor( int key );
void EditorRefreshScreen();
void UpdateCursorPosition();
WindowSize* GetWindowSize();
void EditorProcessKeypress();
void SBFree( StrBuffer* sb );
void Die( const char* message );
void SBAppend( StrBuffer* sb , const char* s , int len );

/*** Init ***/
int main()
{
    EnableRawMode(); // Disables certain terminal behaviors
    EditorInit(); // Initialize default editor settings

    while (1)
    {
        EditorRefreshScreen(); // Refresh terminal window
        EditorProcessKeypress(); // Process each keypress, the user has registered
    }

    return 0;
}

void EditorInit()
{
    // Initialize cursor position
    cursor.x = 0;
    cursor.y = 0;
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

    // Save all keypresses to the char c, if no char is entered exit
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

    WindowSize* pWindow = (WindowSize*) malloc( sizeof( WindowSize ) );

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
        case CTRL_KEY( 'q' ):
            EditorRefreshScreen();
            exit( 0 );
            break;
        case 'w':
        case 'a':
        case 's':
        case 'd':
            MoveCursor( c ); // Handle movement
            break;
    }
}

/*** String Buffer ***/
void SBAppend( StrBuffer* sb , const char* s , int len )
{
    char* new = realloc( sb->b , sb->len + len );

    if (new == NULL) return;

    memcpy( &new[sb->len] , s , len );
    sb->b = new;
    sb->len += len;
}

void SBFree( StrBuffer* sb )
{
    free( sb->b );
}

/*** Output ***/
void ResetCursor()
{
    // Positions the cursor to the top left
    SBAppend( sb , "\x1b[H" , 3 );
}

void HideCursor()
{
    // Tell the terminal to hide the cursor
    SBAppend( sb , "\x1b[?25h" , 6 );
}

void EditorRefreshScreen()
{
    StrBuffer buffer = STR_BUFFER_INIT;
    sb = &buffer;

    HideCursor(); // Hide cursor when keys are pressed
    ResetCursor(); // Move cursor to top left of the terminal

    EditorDrawRows(); // Draw ~ for all the rows in the terminal window

    ResetCursor();
    HideCursor();

    write( STDOUT_FILENO , sb->b , sb->len );
    SBFree( sb );

    UpdateCursorPosition(); // Update cursor position in the terminal
}

void EditorDrawRows()
{
    WindowSize* pConsoleWindow = GetWindowSize();

    int i;
    for (i = 0; i < pConsoleWindow->Rows; i++)
    {
        if (i == pConsoleWindow->Rows / 3)
        {
            char welcome[80]; // Welcome message

            int welcomeLen = snprintf(
                welcome ,
                sizeof( welcome ) ,
                "Pinkz Editor -- Version %s" ,
                EDITOR_VERSION
            );

            // Truncate the length of the welcome message if the window is too small
            if (welcomeLen > pConsoleWindow->Columns) welcomeLen = pConsoleWindow->Columns;

            // Center the welcome message
            int padding = ( pConsoleWindow->Columns - welcomeLen ) / 2;

            // Print the first ~, before the padding
            if (padding)
            {
                SBAppend( sb , "~" , 1 );
                padding--;
            }

            // After the ~, add the padding to center the welcome message
            while (padding--) SBAppend( sb , " " , 1 );

            // Print the welcome message
            SBAppend( sb , welcome , welcomeLen );
        }
        else
        {
            SBAppend( sb , "~" , 1 ); // Print one row

        }

        SBAppend( sb , "\x1b[K" , 3 ); // Clear one line at a time

        if (i < pConsoleWindow->Rows - 1) SBAppend( sb , "\r\n" , 2 );
    }
}

/*** Cursor Movement ***/
void MoveCursor( int key )
{
    switch (key)
    {
        case 'w':
            if (cursor.y > 0) cursor.y--;
            break;
        case 's':
            cursor.y++;
            break;
        case 'a':
            if (cursor.x > 0) cursor.x--;
            break;
        case 'd':
            cursor.x++;
            break;
    }
}

void UpdateCursorPosition()
{
    // Move the console cursor to the new position
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    COORD position = { cursor.x, cursor.y };
    SetConsoleCursorPosition( hConsole , position );
}
