// openurl.c — portable browser launcher
// Build examples:
//   gcc -O2 -o openurl openurl.c
//   cosmocc -O2 -o openurl.com openurl.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define URL "https://www.youtube.com/watch?v=dQw4w9WgXcQ"

int main(void) {
    char cmd[512];
    const char *command;

#if defined(_WIN32)
    command = "cmd.exe /c start \"\" \"" URL "\"";
#elif defined(__APPLE__)
    command = "open '" URL "'";
#elif defined(__linux__)
    command = "xdg-open '" URL "'";
#else
    command = "xdg-open '" URL "' 2>/dev/null";
#endif

    snprintf(cmd, sizeof(cmd), "%s", command);
    fprintf(stderr, "Launching: %s\n", cmd);

    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Failed to open browser (exit %d).\n", rc);
        return 1;
    }

    return 0;
}