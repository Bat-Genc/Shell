// fetch_and_run.c
// Build: cosmocc -O2 -o fetch_and_run.com fetch_and_run.c
//
// Usage:
//   ./fetch_and_run.com
//   ./fetch_and_run.com <URL>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cosmo.h>   // Cosmopolitan: IsWindows(), IsLinux(), IsXnu(), etc.

#define DEFAULT_URL "https://github.com/Bat-Genc/Shell/raw/master/openurl/openurl.com"

int main(int argc, char *argv[]) {
    const char *url = (argc > 1) ? argv[1] : DEFAULT_URL;

    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr,
                "Usage: %s [URL]\n"
                "Downloads and runs a file from a URL.\n"
                "Default URL:\n"
                "  %s\n", argv[0], DEFAULT_URL);
        return 0;
    }

    // Извлечи името на файла от URL-а
    const char *last_slash = strrchr(url, '/');
    const char *filename = last_slash ? last_slash + 1 : "downloaded.com";

    char outfile[512];
    char cmd[4096];

    if (IsWindows()) {
        // Windows: използвай %TEMP%
        const char *tmp = getenv("TEMP");
        if (!tmp) tmp = getenv("TMP");
        if (!tmp) tmp = "C:\\Windows\\Temp";
        snprintf(outfile, sizeof(outfile), "%s\\%s", tmp, filename);
    } else {
        // Linux/macOS: /tmp
        snprintf(outfile, sizeof(outfile), "/tmp/%s", filename);
    }

    // 1. Изтегли файла
    fprintf(stderr, "Downloading %s ...\n", url);
    fprintf(stderr, "Saving to: %s\n", outfile);

    if (IsWindows()) {
        // Windows: PowerShell
        snprintf(cmd, sizeof(cmd),
                 "powershell -NoProfile -Command "
                 "\"Invoke-WebRequest -Uri '%s' -OutFile '%s'\"",
                 url, outfile);
    } else if (IsXnu()) {
        // macOS: curl
        snprintf(cmd, sizeof(cmd),
                 "curl -L -o '%s' '%s'", outfile, url);
    } else {
        // Linux: curl или wget
        snprintf(cmd, sizeof(cmd),
                 "curl -L -o '%s' '%s' || wget -O '%s' '%s'",
                 outfile, url, outfile, url);
    }

    fprintf(stderr, "Running: %s\n", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Download failed (exit %d).\n", rc);
        return 1;
    }

    // 2. Дай права за изпълнение (само Linux/macOS)
    if (!IsWindows()) {
        snprintf(cmd, sizeof(cmd), "chmod +x '%s'", outfile);
        system(cmd);
    }

    // 3. Стартирай файла
    fprintf(stderr, "Running %s ...\n", outfile);

    if (IsWindows()) {
        snprintf(cmd, sizeof(cmd), "\"%s\"", outfile);
    } else {
        snprintf(cmd, sizeof(cmd), "'%s'", outfile);
    }

    fprintf(stderr, "Executing: %s\n", cmd);
    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Execution failed (exit %d).\n", rc);
        return 1;
    }

    return 0;
}