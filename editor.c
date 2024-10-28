#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>
#include <conio.h>

/*** Editor Version ***/
#define EDITOR_VERSION "0.1.0"

/*** Macros for Key Handling ***/
#define CTRL_KEY(k) ((k) & 0x1f) // Mask to identify Ctrl + key combinations
#define STR_BUFFER_INIT {NULL, 0} // Initial empty buffer configuration

/*** Structs ***/
typedef struct
{
    int rows;    // Terminal rows
    int columns; // Terminal columns
} WindowSize;

typedef struct
{
    char* buffer; // Dynamic character buffer
    int length;   // Length of the current buffer
} StringBuffer;

typedef struct
{
    int x; // Cursor position on the x-axis
    int y; // Cursor position on the y-axis
} CursorPosition;

typedef struct
{
    int size;
    char* value;
} TextRow;

/*** Key Definitions for Editor Navigation ***/
enum editorKey
{
    ARROW_LEFT = 1000 ,
    ARROW_RIGHT ,
    ARROW_UP ,
    ARROW_DOWN ,
    PAGE_UP ,
    PAGE_DOWN ,
    HOME_KEY ,
    END_KEY ,
    DELETE_KEY
};

/*** Global Variables ***/
int numOfRows;
TextRow textRow;
HANDLE inputHandle;
CursorPosition cursor;
DWORD originalConsoleMode;
StringBuffer* outputBuffer;

/*** Function Prototypes ***/
int ReadKeyInput();
void UpdateCursor();
void DrawEditorRows();
void ProcessKeypress();
void InitializeEditor();
void EnableRawInputMode();
void DisableRawInputMode();
void MoveCursor( int key );
void RefreshEditorScreen();
WindowSize* GetTerminalSize();
void EditorOpen( char* filename );
void FreeStringBuffer( StringBuffer* buffer );
void HandleFatalError( const char* errorMessage );
ssize_t getline( char** lineptr , size_t* n , FILE* stream );
void AppendToStringBuffer( StringBuffer* buffer , const char* str , int length );

/*** Editor Initialization ***/
int main( int argc , char* argv[] )
{
    EnableRawInputMode(); // Enables raw mode to disable typical terminal behavior
    InitializeEditor();   // Sets initial editor state, e.g., cursor position
    if (argc >= 2) EditorOpen( argv[1] );

    while (1)
    {
        RefreshEditorScreen(); // Update display
        ProcessKeypress();     // Handle key input from the user
    }

    return 0;
}

void InitializeEditor()
{
    // Start cursor at the top-left corner
    cursor.x = 0;
    cursor.y = 0;

    // Terminal defaults
    numOfRows = 0;
}

/*** Terminal Input Handling ***/
void EnableRawInputMode()
{
    inputHandle = GetStdHandle( STD_INPUT_HANDLE );
    DWORD newConsoleMode;

    // Save the original console mode for restoration
    GetConsoleMode( inputHandle , &originalConsoleMode );
    atexit( DisableRawInputMode ); // Ensure raw mode is disabled on exit

    newConsoleMode = originalConsoleMode;

    // Configure raw mode by disabling console features
    newConsoleMode &= ~( ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_PROCESSED_OUTPUT );

    // Apply the new console mode
    if (!SetConsoleMode( inputHandle , newConsoleMode )) HandleFatalError( "EnableRawInputMode" );
}

void DisableRawInputMode()
{
    // Restore the original console mode
    if (!SetConsoleMode( inputHandle , originalConsoleMode )) HandleFatalError( "DisableRawInputMode" );
}

void HandleFatalError( const char* errorMessage )
{
    RefreshEditorScreen();
    perror( errorMessage ); // Display error details
    exit( 1 );
}

int ReadKeyInput()
{
    INPUT_RECORD inputRecord;
    DWORD eventCount;

    while (1)
    {
        ReadConsoleInput( inputHandle , &inputRecord , 1 , &eventCount );
        if (inputRecord.EventType == KEY_EVENT && inputRecord.Event.KeyEvent.bKeyDown)
        {
            switch (inputRecord.Event.KeyEvent.wVirtualKeyCode)
            {
                case VK_LEFT: return ARROW_LEFT;
                case VK_RIGHT: return ARROW_RIGHT;
                case VK_UP: return ARROW_UP;
                case VK_DOWN: return ARROW_DOWN;
                case VK_PRIOR: return PAGE_UP;
                case VK_NEXT: return PAGE_DOWN;
                case VK_HOME: return HOME_KEY;
                case VK_END: return END_KEY;
                case VK_DELETE: return DELETE_KEY;
                default: return inputRecord.Event.KeyEvent.uChar.AsciiChar;
            }
        }
    }
}

WindowSize* GetTerminalSize()
{
    HANDLE outputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WindowSize* terminalSize = (WindowSize*) malloc( sizeof( WindowSize ) );

    if (GetConsoleScreenBufferInfo( outputHandle , &consoleInfo ))
    {
        terminalSize->columns = consoleInfo.srWindow.Right - consoleInfo.srWindow.Left + 1;
        terminalSize->rows = consoleInfo.srWindow.Bottom - consoleInfo.srWindow.Top + 1;
    }

    return terminalSize;
}

/*** File I/O ***/
void EditorOpen( char* filename )
{
    FILE* fp = fopen( filename , "r" );

    if (!fp) HandleFatalError( "Failed to open file" );

    char* line = NULL;
    size_t linecap = 0;
    ssize_t linelen = 0;

    // Read each line from the file
    while (( linelen = getline( &line , &linecap , fp ) ) != -1)
    {
        while (linelen > 0 && ( line[linelen - 1] == '\n' || line[linelen - 1] == '\r' ))
            linelen--;  // Trim newline or carriage return at the end

        textRow.size = linelen;
        textRow.value = malloc( linelen + 1 );

        if (textRow.value == NULL) HandleFatalError( "Failed to allocate memory for line" );

        memcpy( textRow.value , line , linelen );

        textRow.value[linelen] = '\0';
        numOfRows = 1;
    }

    free( line );
    fclose( fp );
}

/*** Key Press Processing ***/
void ProcessKeypress()
{
    WindowSize* terminalSize = GetTerminalSize();

    int key = ReadKeyInput();
    switch (key)
    {
        case CTRL_KEY( 'q' ):
            RefreshEditorScreen();
            exit( 0 ); // Exit on Ctrl-Q
            break;

        case HOME_KEY:
            cursor.x = 0; // Move to the start of the row
            break;

        case END_KEY:
            cursor.x = terminalSize->columns - 1; // Move cursor to the end of the row
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int rows = terminalSize->rows;

                while (rows--)
                {
                    // Move the cursor to the top and bottom of the terminal window
                    MoveCursor( key == PAGE_UP ? ARROW_UP : ARROW_DOWN );
                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            MoveCursor( key ); // Adjust cursor position based on arrow key
            break;
    }
}

/*** Dynamic String Buffer Operations ***/
void AppendToStringBuffer( StringBuffer* buffer , const char* str , int length )
{
    char* newBuffer = realloc( buffer->buffer , buffer->length + length );

    if (newBuffer == NULL) return;

    memcpy( &newBuffer[buffer->length] , str , length );
    buffer->buffer = newBuffer;
    buffer->length += length;
}

void FreeStringBuffer( StringBuffer* buffer )
{
    free( buffer->buffer );
}

/*** Screen Rendering ***/
void ResetCursorPosition()
{
    AppendToStringBuffer( outputBuffer , "\x1b[H" , 3 ); // Move cursor to top-left
}

void HideCursor()
{
    AppendToStringBuffer( outputBuffer , "\x1b[?25h" , 6 ); // Make cursor invisible
}

void RefreshEditorScreen()
{
    StringBuffer buffer = STR_BUFFER_INIT;
    outputBuffer = &buffer;

    HideCursor();
    ResetCursorPosition();

    DrawEditorRows(); // Render each row

    ResetCursorPosition();
    HideCursor();

    write( STDOUT_FILENO , outputBuffer->buffer , outputBuffer->length );
    FreeStringBuffer( outputBuffer );

    UpdateCursor(); // Update terminal cursor position
}

void DrawEditorRows()
{
    WindowSize* terminalSize = GetTerminalSize();

    for (int i = 0; i < terminalSize->rows; i++)
    {
        if (i >= terminalSize->rows)
        {
            if (i == terminalSize->rows / 3)
            {
                char welcome[80];

                int welcomeLength = snprintf(
                    welcome ,
                    sizeof( welcome ) ,
                    "Pinkz Editor -- Version %s" ,
                    EDITOR_VERSION
                );

                if (welcomeLength > terminalSize->columns) welcomeLength = terminalSize->columns;

                int padding = ( terminalSize->columns - welcomeLength ) / 2;

                if (padding)
                {
                    AppendToStringBuffer( outputBuffer , "~" , 1 );
                    padding--;
                }

                while (padding--) AppendToStringBuffer( outputBuffer , " " , 1 );

                AppendToStringBuffer( outputBuffer , welcome , welcomeLength );
            }
            else
            {
                AppendToStringBuffer( outputBuffer , "~" , 1 ); // Add filler for empty rows
            }
        }
        else
        {
            int len = textRow.size;

            if (len > terminalSize->columns) len = terminalSize->columns;

            AppendToStringBuffer( outputBuffer , textRow.value , len );
        }

        AppendToStringBuffer( outputBuffer , "\x1b[K" , 3 ); // Clear line

        if (i < terminalSize->rows - 1) AppendToStringBuffer( outputBuffer , "\r\n" , 2 );
    }
}

/*** Cursor Management ***/
void MoveCursor( int key )
{
    WindowSize* terminalSize = GetTerminalSize();

    switch (key)
    {
        case ARROW_UP:
            if (cursor.y > 0) cursor.y--;
            break;
        case ARROW_DOWN:
            if (cursor.y < terminalSize->rows - 1) cursor.y++;
            break;
        case ARROW_LEFT:
            if (cursor.x > 0) cursor.x--;
            break;
        case ARROW_RIGHT:
            if (cursor.x < terminalSize->columns - 1) cursor.x++;
            break;
    }
}

void UpdateCursor()
{
    HANDLE outputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    COORD position = { cursor.x, cursor.y };
    SetConsoleCursorPosition( outputHandle , position );
}

/*** Read lines from a file ***/
ssize_t getline( char** lineptr , size_t* n , FILE* stream )
{
    if (*lineptr == NULL || *n == 0)
    {
        *n = 128;  // Starting buffer size
        *lineptr = malloc( *n );
        if (*lineptr == NULL) return -1;
    }

    char* ptr = *lineptr;
    int c;
    size_t i = 0;

    while (( c = fgetc( stream ) ) != EOF && c != '\n')
    {
        if (i + 1 >= *n)
        {
            *n *= 2;
            char* new_ptr = realloc( *lineptr , *n );
            if (new_ptr == NULL) return -1;
            *lineptr = new_ptr;
            ptr = *lineptr + i;
        }
        *ptr++ = (char) c;
        i++;
    }

    if (i == 0 && c == EOF) return -1;

    *ptr = '\0';
    return i;
}