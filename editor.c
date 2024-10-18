#include <stdio.h>
#include <windows.h>

DWORD originalMode;

void EnableRawMode();
void DisableRawMode();

int main()
{
    EnableRawMode();

    char c;
    while ((c = getchar()) != EOF && (c = getchar()) != 'q');
    printf("%c", c);

    return 0;
}

void EnableRawMode()
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD newMode;

    // Current console mode
    GetConsoleMode(hStdin, &originalMode);

    // At exit, disable raw mode
    atexit(DisableRawMode);

    // Copy the default console mode to modify that
    newMode = originalMode;

    // Turn off ECHO mode
    newMode &= ~ENABLE_ECHO_INPUT;

    // Change the current mode with the new configuration
    SetConsoleMode(hStdin, newMode);
}

void DisableRawMode()
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Set the console to the default mode
    SetConsoleMode(hStdin, originalMode);
}