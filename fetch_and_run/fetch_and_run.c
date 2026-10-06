// fetch_and_run.c
// Build: cosmocc -O2 -o fetch_and_run.com fetch_and_run.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __COSMOPOLITAN__
  #include <cosmo.h>   // IsWindows(), IsLinux(), IsXnu(), etc.
#endif

#define DEFAULT_URL "https://github.com/Bat-Genc/Shell/raw/master/openurl/openurl.com"

int main(int argc, char *argv[]) {
    const char *url = (argc > 1) ? argv[1] : DEFAULT_URL;

    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr, "Usage: %s [URL]\n", argv[0]);
        return 0;
    }

    const char *last_slash = strrchr(url, '/');
    const char *filename = last_slash ? last_slash + 1 : "downloaded.com";

    char outfile[512];
    char cmd[4096];

#ifdef __COSMOPOLITAN__
    if (IsWindows()) {
#else
    #ifdef _WIN32
    if (1) {
    #else
    if (0) {
    #endif
#endif
        const char *tmp = getenv("TEMP");
        if (!tmp) tmp = getenv("TMP");
        if (!tmp) tmp = "C:/Windows/Temp";
        snprintf(outfile, sizeof(outfile), "%s/%s", tmp, filename);
        // Замени backslashes с forward slashes
        for (char *p = outfile; *p; p++) if (*p == '\\') *p = '/';
    } else {
        snprintf(outfile, sizeof(outfile), "/tmp/%s", filename);
    }

    fprintf(stderr, "Downloading %s ...\n", url);
    fprintf(stderr, "Saving to: %s\n", outfile);

    // 1. Изтегли файла
#ifdef __COSMOPOLITAN__
    if (IsWindows()) {
#else
    #ifdef _WIN32
    if (1) {
    #else
    if (0) {
    #endif
#endif
        snprintf(cmd, sizeof(cmd),
                 "cmd.exe /c curl.exe -L -o \"%s\" \"%s\"",
                 outfile, url);
    }
#ifdef __COSMOPOLITAN__
    else if (IsXnu()) {
#else
    #ifdef __APPLE__
    else if (1) {
    #else
    else if (0) {
    #endif
#endif
        snprintf(cmd, sizeof(cmd), "curl -L -o '%s' '%s'", outfile, url);
    } else {
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

    // 2. chmod (само на Linux/macOS)
#ifdef __COSMOPOLITAN__
    if (!IsWindows()) {
#else
    #ifndef _WIN32
    if (1) {
    #else
    if (0) {
    #endif
#endif
        snprintf(cmd, sizeof(cmd), "chmod +x '%s'", outfile);
        system(cmd);
    }

    // 3. Стартирай файла
    fprintf(stderr, "Running %s ...\n", outfile);

#ifdef __COSMOPOLITAN__
    if (IsWindows()) {
#else
    #ifdef _WIN32
    if (1) {
    #else
    if (0) {
    #endif
#endif
        snprintf(cmd, sizeof(cmd), "cmd.exe /c \"%s\"", outfile);
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