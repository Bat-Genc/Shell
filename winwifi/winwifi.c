// winwifi.c — show all saved WiFi profiles + passwords
// Windows only.
// Build:  cosmocc -O2 -o winwifi.com winwifi.c
// Run:    winwifi.com         (run as Administrator!)
//
// Usage:
//   winwifi.com       show all saved WiFi networks and their passwords
//   winwifi.com -h    show help
//
// Requires: Administrator privileges (for key=clear to return the password)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROFILES 128
#define NETSH "C:/Windows/System32/netsh.exe"

// =============================================================================
// Get all saved WiFi profile names.
// Fills 'profiles[][128]', returns count.
// =============================================================================
static int get_profile_list(char profiles[][128], int max) 
{
    FILE *fp = popen(NETSH " wlan show profiles", "r");
    if (!fp) return 0;

    int count = 0;
    char line[512];
    int service_down = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Detect service not running
        if (strstr(line, "not running") || strstr(line, "not found")) {
            service_down = 1;
        }

        // Line format: "    All User Profile     : MyWiFi"
        char *p = strstr(line, "All User Profile");
        if (!p) continue;

        char *colon = strchr(p, ':');
        if (!colon) continue;
        colon++;
        while (*colon == ' ' || *colon == '\t') colon++;

        size_t len = strlen(colon);
        while (len > 0 && (colon[len-1] == '\n' || colon[len-1] == '\r' ||
                           colon[len-1] == ' ' || colon[len-1] == '\t'))
            colon[--len] = '\0';

        if (len > 0 && count < max) {
            strncpy(profiles[count], colon, 127);
            profiles[count][127] = '\0';
            count++;
        }
    }
    pclose(fp);

    if (service_down) {
        fprintf(stderr, "\n  ERROR: WiFi service (wlansvc) is not running.\n");
        fprintf(stderr, "  Fix: run this in admin cmd:\n");
        fprintf(stderr, "    net start wlansvc\n\n");
    }
    return count;
}
// =============================================================================
// Get password for one profile.
// Returns 0 if password found, -1 otherwise (e.g. open network).
// =============================================================================
static int get_profile_password(const char *name, char *password, size_t n) {
    password[0] = '\0';

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             NETSH " wlan show profile name=\"%s\" key=clear",
             name);

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "Key Content")) {
            char *c = strchr(line, ':');
            if (c) {
                c++;
                while (*c == ' ' || *c == '\t') c++;
                size_t len = strlen(c);
                while (len > 0 && (c[len-1] == '\n' || c[len-1] == '\r' ||
                                   c[len-1] == ' ' || c[len-1] == '\t'))
                    c[--len] = '\0';
                strncpy(password, c, n - 1);
                password[n - 1] = '\0';
                pclose(fp);
                return password[0] ? 0 : -1;
            }
        }
    }
    pclose(fp);
    return -1;
}
// =============================================================================
// MAIN
// =============================================================================
int main(int argc, char *argv[]) {
    // Help
    if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        printf("Usage: %s\n\n", argv[0]);
        printf("  Shows all saved WiFi networks and their passwords.\n");
        printf("  Requires Administrator privileges.\n\n");
        printf("  Run as admin:\n");
        printf("    - Press Win+X\n");
        printf("    - Choose 'Terminal (Admin)' or 'PowerShell (Admin)'\n");
        printf("    - Then run: .\\%s\n", argv[0]);
        return 0;
    }

    printf("\n");
    printf("  ========================================\n");
    printf("  Saved WiFi Networks + Passwords\n");
    printf("  ========================================\n\n");

    // 1. Get list of all saved profiles
    char profiles[MAX_PROFILES][128];
    int n = get_profile_list(profiles, MAX_PROFILES);

    if (n == 0) {
        printf("  No saved WiFi profiles found.\n");
        printf("  Make sure you're running as Administrator.\n\n");
        return 1;
    }

    printf("  Found %d saved profile(s):\n\n", n);

    // 2. For each profile, try to get its password
    int shown = 0;
    for (int i = 0; i < n; i++) {
        char password[256];
        if (get_profile_password(profiles[i], password, sizeof(password)) == 0) {
            printf("  %-32s %s\n", profiles[i], password);
            shown++;
        }
        // If no password (open network), skip silently
    }

    if (shown == 0) {
        printf("  (No passwords found.)\n\n");
        printf("  Possible reasons:\n");
        printf("    - Not running as Administrator\n");
        printf("    - All saved networks are open (no password)\n");
        printf("    - WiFi service (wlansvc) is not running\n");
    } else {
        printf("\n  Total: %d network(s) with password\n", shown);
    }

    printf("\n");
    return 0;
}