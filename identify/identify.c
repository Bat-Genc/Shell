#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cosmo.h>

int main(void) {
    const char *shell = getenv("SHELL");
    if (!shell) shell = getenv("COMSPEC");
    if (!shell) shell = "unknown";

    const char *term = getenv("TERM_PROGRAM");
    if (!term) term = getenv("TERM");
    if (!term && getenv("WT_SESSION")) term = "Windows Terminal";
    if (!term) term = "unknown";

    // ИСТИНСКА runtime проверка на OS
    const char *platform = "Unknown";
    if (IsWindows())      platform = "Windows";
    else if (IsLinux())   platform = "Linux";
    else if (IsXnu())     platform = "macOS";
    else if (IsFreebsd()) platform = "FreeBSD";
    else if (IsOpenbsd()) platform = "OpenBSD";
    else if (IsNetbsd())  platform = "NetBSD";

    // Архитектура — compile-time
    const char *arch = "unknown";
    #ifdef __x86_64__
        arch = "x86_64";
    #elif defined(__aarch64__)
        arch = "aarch64";
    #elif defined(__i386__)
        arch = "i386";
    #elif defined(__arm__)
        arch = "arm";
    #endif

    printf("Platform : %s\n", platform);
    printf("Arch     : %s\n", arch);
    printf("Shell    : %s\n", shell);
    printf("Terminal : %s\n", term);

    return 0;
}
