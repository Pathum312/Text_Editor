/*** Includes ***/
#include <stdio.h>
#include <windows.h>
#include <ctype.h>
#include <unistd.h>

/*** Data ***/
HANDLE hStdin;
DWORD originalMode;

/*** Prototypes ***/
void EnableRawMode();
void DisableRawMode();
void Die( const char* message );

/*** Init ***/
int main()
{
    EnableRawMode();

    char c;

    while (1)
    {
        // If the char is the EOF, then exit.
        if (read( STDIN_FILENO , &c , 1 ) == -1 && errno != EAGAIN) Die( "Read" );

        /*
            If the entered char is not a control char,
            then display that keypress.
        */
        !iscntrl( c ) ? printf( "%c\r\n" , c ) : printf( "%d (%c)\r\n" , c , c );

        // If the char is q, exit the editor.
        if (c == 'q') break;
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
    perror( message );
    exit( 1 );
}