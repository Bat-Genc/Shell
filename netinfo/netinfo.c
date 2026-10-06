// netinfo.c — portable network info tool
// Build: cosmocc -O2 -o netinfo.com netinfo.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cosmo.h>

static void label(const char *l) {
    printf("  %-22s", l);
    fflush(stdout);
}

static void trim(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' ||
                       s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i > 0) memmove(s, s + i, strlen(s + i) + 1);
}

static int run(const char *cmd, char *out, size_t n) {
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;
    if (!fgets(out, n, fp)) { pclose(fp); return -1; }
    pclose(fp);
    trim(out);
    return (out[0]) ? 0 : -1;
}

static void show(const char *lbl, const char *cmd) {
    label(lbl);
    char buf[2048];
    if (run(cmd, buf, sizeof(buf)) == 0) printf("%s\n", buf);
    else printf("N/A\n");
}

// ===== WINDOWS =====
static void show_windows(void) {
    // Forward slashes work on Windows too!
    FILE *fp = popen("C:/Windows/System32/ipconfig.exe /all", "r");
    if (!fp) {
        printf("  (failed to run ipconfig)\n");
        return;
    }

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        const char *keys[] = {
            "IPv4 Address", "IPv6 Address", "Physical Address",
            "Default Gateway", "DNS Servers", NULL
        };
        for (int i = 0; keys[i]; i++) {
            if (strstr(line, keys[i])) {
                printf("  %s", line);
                break;
            }
        }
    }
    pclose(fp);
}

// ===== WiFi парола =====
static int get_wifi_password(char *psk, size_t n) {
    psk[0] = '\0';
    FILE *fp = popen("LANG=C /usr/bin/nmcli -s dev wifi show-password 2>/dev/null", "r");
    if (!fp) return -1;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        const char *p = strstr(line, "Password:");
        if (p) {
            p += 9;
            while (*p == ' ' || *p == '\t') p++;
            strncpy(psk, p, n - 1);
            psk[n - 1] = '\0';
            trim(psk);
            break;
        }
    }
    pclose(fp);
    return psk[0] ? 0 : -1;
}

// ===== LINUX =====
static void show_linux(int show_password, const char *prog) {
    show("IPv4 Address",
         "ip -4 -br addr show up scope global | grep -v 'docker\\|veth\\|lo' | awk '{print $3}' | cut -d/ -f1");
    show("IPv6 Address",
         "ip -6 -br addr show up | grep -v 'docker\\|veth\\|lo' | grep -v '^::1' | awk '{print $3}' | cut -d/ -f1");
    show("Hardware Address",
         "ip -br link show | grep LOWER_UP | grep -v 'docker\\|veth\\|lo' | awk '{print toupper($3)}'");

    printf("  ----------------------------------------\n");

    // WiFi: чети ВСИЧКИ редове, намери този с *
    char buf[2048] = "";
    int ok = -1;
    FILE *fp = popen("LANG=C /usr/bin/nmcli -t -f IN-USE,SSID,SIGNAL,RATE,CHAN,FREQ,SECURITY dev wifi 2>/dev/null", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '*') {
                strncpy(buf, line, sizeof(buf) - 1);
                buf[sizeof(buf) - 1] = '\0';
                trim(buf);
                ok = 0;
                break;
            }
        }
        pclose(fp);
    }

    if (ok == 0 && buf[0] == '*') {
        char *fields[8] = {0};
        int nf = 0;
        char *p = buf, *start = p;
        while (*p && nf < 7) {
            if (*p == '\\' && *(p+1)) { p += 2; continue; }
            if (*p == ':') { *p = '\0'; fields[nf++] = start; start = p + 1; }
            p++;
        }
        fields[nf++] = start;

        char *ssid     = (nf > 1) ? fields[1] : "";
        char *signal   = (nf > 2) ? fields[2] : "";
        char *rate     = (nf > 3) ? fields[3] : "";
        char *chan     = (nf > 4) ? fields[4] : "";
        char *freq     = (nf > 5) ? fields[5] : "";
        char *security = (nf > 6) ? fields[6] : "";

        trim(ssid); trim(signal); trim(rate);
        trim(chan); trim(freq); trim(security);

        const char *band = "N/A";
        int f = atoi(freq);
        if (f >= 2400 && f < 2500)      band = "2.4 GHz";
        else if (f >= 5000 && f < 6000) band = "5 GHz";
        else if (f >= 6000 && f < 7200) band = "6 GHz";

        char sec_display[128];
        if (!security[0] || !strcmp(security, "--"))
            snprintf(sec_display, sizeof(sec_display), "Open (no password)");
        else
            snprintf(sec_display, sizeof(sec_display), "%s", security);

        char sig_display[64];
        snprintf(sig_display, sizeof(sig_display), "%s%%", signal);

        label("SSID");        printf("%s\n", ssid[0] ? ssid : "N/A");
        label("Signal");      printf("%s\n", signal[0] ? sig_display : "N/A");
        label("Link Speed");  printf("%s\n", rate[0] ? rate : "N/A");
        label("Channel");     printf("%s\n", chan[0] ? chan : "N/A");
        label("Frequency");   printf("%s\n", freq[0] ? freq : "N/A");
        label("Band");        printf("%s\n", band);
        label("Security");    printf("%s\n", sec_display);

        int is_open = (!security[0] || !strcmp(security, "--"));

        if (is_open) {
            label("Password");
            printf("(open network - no password)\n");
        } else if (show_password) {
            if (geteuid() != 0) {
                label("Password");
                printf("(needs sudo: sudo %s -p)\n", prog);
            } else {
                char psk[256] = "";
                if (get_wifi_password(psk, sizeof(psk)) == 0)
                    { label("Password"); printf("%s\n", psk); }
                else
                    { label("Password"); printf("(not found)\n"); }
            }
        } else {
            label("Password");
            printf("**********   (use -p to show)\n");
        }
    } else {
        label("WiFi");
        printf("no active wifi\n");
    }

    printf("  ----------------------------------------\n");
    show("Default Route", "ip route | grep default | awk '{print $3}'");
    show("DNS", "cat /etc/resolv.conf | grep nameserver | awk '{print $2}'");
}

int main(int argc, char *argv[]) {
    int show_password = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("  -p, --password   Show WiFi password (Linux only, needs sudo)\n");
            printf("  -h, --help       Show this help\n");
            return 0;
        }
        if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--password"))
            show_password = 1;
    }

    const char *platform = "Unknown";
    if (IsWindows())      platform = "Windows";
    else if (IsLinux())   platform = "Linux";
    else if (IsXnu())     platform = "macOS";

    printf("\n");
    printf("  Platform              %s\n", platform);
    printf("  ----------------------------------------\n");

    if (IsWindows()) {
        show_windows();
    } else if (IsLinux()) {
        show_linux(show_password, argv[0]);
    } else if (IsXnu()) {
        show("IPv4 Address", "ifconfig | grep 'inet ' | grep -v 127.0.0.1 | awk '{print $2}'");
        show("Hardware Address", "ifconfig | grep ether | awk '{print toupper($2)}'");
    }

    printf("\n");
    return 0;
}