// fetch_and_run.c
// Build: cosmocc -O2 -o fetch_and_run.com fetch_and_run.c
//
// Usage:
//   ./fetch_and_run.com              # uses DEFAULT_URL
//   ./fetch_and_run.com <URL>        # uses custom URL
//   ./fetch_and_run.com --help

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE 256
#define DEFAULT_URL "https://github.com/Bat-Genc/Shell/raw/master/openurl/openurl.com"

int main(int argc, char *argv[]) {
    const char *url;

    // Help flag
    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr,
                "Usage: %s [URL]\n"
                "Downloads and runs a file from URL.\n"
                "If no URL is given, uses the default:\n"
                "  %s\n",
                argv[0], DEFAULT_URL);
        return 0;
    }

    // Use default URL if no argument
    if (argc < 2) {
        url = DEFAULT_URL;
        fprintf(stderr, "No URL given, using default:\n  %s\n", url);
    } else {
        url = argv[1];
    }

    // Extract filename from URL
    const char *last_slash = strrchr(url, '/');
    const char *filename = last_slash ? last_slash + 1 : "downloaded.com";

    char cmd[4096];
    char outfile[MAX_FILE];

    // Temp directory
    const char *tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = getenv("TMP");
    if (!tmpdir) tmpdir = "/tmp";

    snprintf(outfile, sizeof(outfile), "%s/%s", tmpdir, filename);

    // 1. Download
    fprintf(stderr, "Downloading %s ...\n", url);

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "curl -L -o \"%s\" \"%s\"", outfile, url);
#else
    snprintf(cmd, sizeof(cmd),
             "curl -L -o '%s' '%s' || wget -O '%s' '%s'",
             outfile, url, outfile, url);
#endif

    fprintf(stderr, "Running: %s\n", cmd);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Download failed (exit %d).\n", rc);
        return 1;
    }

    // 2. Make executable (Linux only)
#ifndef _WIN32
    snprintf(cmd, sizeof(cmd), "chmod 755 '%s'", outfile);
    fprintf(stderr, "Running: %s\n", cmd);
    system(cmd);
#endif

    // 3. Execute
    fprintf(stderr, "Running %s ...\n", outfile);

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "\"%s\"", outfile);
#else
    snprintf(cmd, sizeof(cmd), "'%s'", outfile);
#endif

    fprintf(stderr, "Executing: %s\n", cmd);
    rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Execution failed (exit %d).\n", rc);
        return 1;
    }

    return 0;
}