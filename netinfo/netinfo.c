// netinfo.c — portable network info tool
// Build: cosmocc -O2 -o netinfo.com netinfo.c
//
// Usage:
//   ./netinfo.com              show info (password hidden)
//   sudo ./netinfo.com -p      show WiFi password (Linux)
//   netinfo.com -p             show WiFi password (Windows, run as admin)
//   ./netinfo.com -s           run internet speed test
//   sudo ./netinfo.com -p -s   show password + speed test
//   ./netinfo.com -h           help

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

// ===== SPEED TEST (runtime OS detection) =====
static void show_speedtest(void) {
    printf("  ----------------------------------------\n");
    printf("  Speed Test (Cloudflare)\n");
    printf("  ----------------------------------------\n");

    const char *curl;
    const char *null_dev;

    if (IsWindows()) {
        curl = "C:/Windows/System32/curl.exe";
        null_dev = "NUL";
    } else {
        curl = "curl";
        null_dev = "/dev/null";
    }

    char cmd[2048];
    char buf[256];

    // ===== Download test (25 MB) =====
    label("Download");
    snprintf(cmd, sizeof(cmd),
             "%s -o %s -s -w \"%%{speed_download}\" "
             "\"https://speed.cloudflare.com/__down?bytes=25000000\"",
             curl, null_dev);

    if (run(cmd, buf, sizeof(buf)) == 0) {
        double bytes_per_sec = atof(buf);
        double mbps = bytes_per_sec * 8 / 1000000.0;
        double mbs  = bytes_per_sec / 1000000.0;
        printf("%.2f Mbps  (%.2f MB/s)\n", mbps, mbs);
    } else {
        printf("N/A\n");
    }

    // ===== Upload test (5 MB) =====
    label("Upload");

    if (IsWindows()) {
        snprintf(cmd, sizeof(cmd),
                 "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe "
                 "-NoProfile -Command "
                 "\"$data = New-Object byte[] 5242880; "
                 "$sw = [System.Diagnostics.Stopwatch]::StartNew(); "
                 "Invoke-WebRequest -Uri 'https://speed.cloudflare.com/__up' "
                 "-Method POST -Body $data -UseBasicParsing | Out-Null; "
                 "$sw.Stop(); "
                 "Write-Host (5242880 / $sw.Elapsed.TotalSeconds)\"");
    } else {
        snprintf(cmd, sizeof(cmd),
                 "dd if=/dev/zero bs=1M count=5 2>/dev/null | "
                 "%s -X POST -s -w \"%%{speed_upload}\" "
                 "--data-binary @- \"https://speed.cloudflare.com/__up\"",
                 curl);
    }

    if (run(cmd, buf, sizeof(buf)) == 0) {
        double bytes_per_sec = atof(buf);
        double mbps = bytes_per_sec * 8 / 1000000.0;
        double mbs  = bytes_per_sec / 1000000.0;
        printf("%.2f Mbps  (%.2f MB/s)\n", mbps, mbs);
    } else {
        printf("N/A\n");
    }

    // ===== Ping test =====
    label("Ping");
    snprintf(cmd, sizeof(cmd),
             "%s -o %s -s -w \"%%{time_total}\" "
             "\"https://speed.cloudflare.com/__down?bytes=1\"",
             curl, null_dev);

    if (run(cmd, buf, sizeof(buf)) == 0) {
        double t = atof(buf);
        printf("%.2f ms\n", t * 1000);
    } else {
        printf("N/A\n");
    }
}

// ===== Determine active connection type (Linux) =====
static int get_active_type(char *iface, size_t n) {
    iface[0] = '\0';
    FILE *fp = popen(
        "LANG=C /usr/bin/nmcli -t -f TYPE,DEVICE connection show --active 2>/dev/null",
        "r");
    if (!fp) return -1;

    char line[256];
    int result = -1;
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (strncmp(line, "802-11-wireless", 15) == 0 ||
            strncmp(line, "wifi:", 5) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                strncpy(iface, colon + 1, n - 1);
                iface[n-1] = '\0';
                trim(iface);
            }
            result = 0;
            break;
        }
        if (strncmp(line, "802-3-ethernet", 14) == 0 ||
            strncmp(line, "ethernet:", 9) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                strncpy(iface, colon + 1, n - 1);
                iface[n-1] = '\0';
                trim(iface);
            }
            result = 1;
            break;
        }
    }
    pclose(fp);
    return result;
}

// ===== WiFi password (Linux, needs sudo) =====
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

// ===== Forward =====
static void show_windows_ethernet(void);

// ===== WINDOWS =====
static void show_windows(int show_password) {
    FILE *fp = popen("C:/Windows/System32/ipconfig.exe /all", "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            char *p = line;
            while (*p == ' ' || *p == '\t') p++;

            const char *keys[] = {
                "Host Name", "IPv4 Address", "IPv6 Address",
                "Link-local IPv6 Address",
                "Physical Address", "Default Gateway", "DNS Servers",
                "DHCP Server", "Subnet Mask", NULL
            };
            for (int i = 0; keys[i]; i++) {
                if (strncmp(p, keys[i], strlen(keys[i])) == 0 && strchr(p, ':')) {
                    char *colon = strchr(p, ':');
                    *colon = '\0';
                    char *key = p;
                    char *val = colon + 1;
                    while (*val == ' ') val++;

                    size_t len = strlen(val);
                    while (len > 0 && (val[len-1] == '\n' || val[len-1] == '\r'))
                        val[--len] = '\0';

                    len = strlen(key);
                    while (len > 0 && (key[len-1] == ' ' || key[len-1] == '\t'))
                        key[--len] = '\0';

                    printf("  %-22s%s\n", key, val);
                    break;
                }
            }
        }
        pclose(fp);
    }

    printf("  ----------------------------------------\n");

    fp = popen("C:/Windows/System32/netsh.exe wlan show interfaces 2>nul", "r");
    if (!fp) {
        show_windows_ethernet();
        return;
    }

    char line[1024];
    char ssid[128] = "";
    char bssid[64] = "";
    char signal[32] = "";
    char rate_rx[64] = "";
    char rate_tx[64] = "";
    char channel[32] = "";
    char band[32] = "";
    char radio[64] = "";
    char cipher[64] = "";
    char auth[64] = "";
    char state[64] = "";

    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        char *colon = strchr(p, ':');
        if (!colon) continue;
        *colon = '\0';
        char *key = p;
        char *val = colon + 1;
        while (*val == ' ') val++;

        size_t len = strlen(val);
        while (len > 0 && (val[len-1] == '\n' || val[len-1] == '\r' ||
                           val[len-1] == ' ' || val[len-1] == '\t'))
            val[--len] = '\0';

        len = strlen(key);
        while (len > 0 && (key[len-1] == ' ' || key[len-1] == '\t'))
            key[--len] = '\0';

        if      (!strcmp(key, "SSID"))                  strncpy(ssid, val, sizeof(ssid)-1);
        else if (!strcmp(key, "BSSID"))                 strncpy(bssid, val, sizeof(bssid)-1);
        else if (!strcmp(key, "Signal"))                strncpy(signal, val, sizeof(signal)-1);
        else if (!strcmp(key, "Receive rate (Mbps)"))   strncpy(rate_rx, val, sizeof(rate_rx)-1);
        else if (!strcmp(key, "Transmit rate (Mbps)"))  strncpy(rate_tx, val, sizeof(rate_tx)-1);
        else if (!strcmp(key, "Channel"))               strncpy(channel, val, sizeof(channel)-1);
        else if (!strcmp(key, "Band"))                  strncpy(band, val, sizeof(band)-1);
        else if (!strcmp(key, "Radio type"))            strncpy(radio, val, sizeof(radio)-1);
        else if (!strcmp(key, "Authentication"))        strncpy(auth, val, sizeof(auth)-1);
        else if (!strcmp(key, "Cipher"))                strncpy(cipher, val, sizeof(cipher)-1);
        else if (!strcmp(key, "State"))                 strncpy(state, val, sizeof(state)-1);
    }
    pclose(fp);

    if (ssid[0]) {
        label("Connection");    printf("WiFi\n");
        label("SSID");          printf("%s\n", ssid);
        if (bssid[0])    { label("BSSID");          printf("%s\n", bssid); }
        if (state[0])    { label("State");          printf("%s\n", state); }
        if (signal[0])   { label("Signal");         printf("%s\n", signal); }
        if (radio[0])    { label("Radio Type");     printf("%s\n", radio); }
        if (rate_rx[0])  { label("RX Rate");        printf("%s Mbps\n", rate_rx); }
        if (rate_tx[0])  { label("TX Rate");        printf("%s Mbps\n", rate_tx); }
        if (channel[0])  { label("Channel");        printf("%s\n", channel); }
        if (band[0])     { label("Band");           printf("%s\n", band); }
        if (auth[0])     { label("Authentication"); printf("%s\n", auth); }
        if (cipher[0])   { label("Cipher");         printf("%s\n", cipher); }

        if (show_password) {
            label("Password");
            char cmd[512];
            snprintf(cmd, sizeof(cmd),
                     "C:/Windows/System32/netsh.exe wlan show profile name=\"%s\" key=clear",
                     ssid);
            FILE *pf = popen(cmd, "r");
            if (pf) {
                char pl[1024];
                int found = 0;
                while (fgets(pl, sizeof(pl), pf)) {
                    if (strstr(pl, "Key Content")) {
                        char *c = strchr(pl, ':');
                        if (c) {
                            c++;
                            while (*c == ' ') c++;
                            size_t l = strlen(c);
                            while (l > 0 && (c[l-1] == '\n' || c[l-1] == '\r'))
                                c[--l] = '\0';
                            printf("%s\n", c);
                            found = 1;
                            break;
                        }
                    }
                }
                if (!found) printf("(not found - run as admin)\n");
                pclose(pf);
            } else {
                printf("(failed)\n");
            }
        } else {
            label("Password");
            printf("**********   (use -p to show)\n");
        }
    } else {
        show_windows_ethernet();
    }
}

// ===== Windows Ethernet details =====
static void show_windows_ethernet(void) {
    label("Connection");
    printf("Ethernet\n");

    FILE *fp = popen(
        "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe "
        "-NoProfile -Command "
        "\"Get-NetAdapter | Where-Object {$_.Status -eq 'Up'} | "
        "Select-Object -First 1 -Property Name,LinkSpeed,MtuSize,MacAddress | "
        "Format-List\"",
        "r");

    if (fp) {
        char wl[512];
        char speed[64] = "";
        char mtu[32] = "";
        char mac[64] = "";

        while (fgets(wl, sizeof(wl), fp)) {
            char *p = wl;
            while (*p == ' ' || *p == '\t') p++;

            char *colon = strchr(p, ':');
            if (!colon) continue;
            *colon = '\0';
            char *key = p;
            char *val = colon + 1;
            while (*val == ' ') val++;

            size_t len = strlen(val);
            while (len > 0 && (val[len-1] == '\n' || val[len-1] == '\r' ||
                               val[len-1] == ' '))
                val[--len] = '\0';

            len = strlen(key);
            while (len > 0 && (key[len-1] == ' ' || key[len-1] == '\t'))
                key[--len] = '\0';

            if      (!strcmp(key, "LinkSpeed"))   strncpy(speed, val, sizeof(speed)-1);
            else if (!strcmp(key, "MtuSize"))     strncpy(mtu, val, sizeof(mtu)-1);
            else if (!strcmp(key, "MacAddress"))  strncpy(mac, val, sizeof(mac)-1);
        }
        pclose(fp);

        if (speed[0]) { label("Link Speed"); printf("%s\n", speed); }
        if (mtu[0])   { label("MTU");        printf("%s\n", mtu); }
        if (mac[0])   { label("MAC");        printf("%s\n", mac); }
    }
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

    char iface[64] = "";
    int conn_type = get_active_type(iface, sizeof(iface));

    if (conn_type == 0) {
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

            label("Connection");  printf("WiFi (%s)\n", iface);
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
            label("WiFi");  printf("no active wifi\n");
        }

    } else if (conn_type == 1) {
        char cmd[256];
        label("Connection");  printf("Ethernet (%s)\n", iface);

        snprintf(cmd, sizeof(cmd),
                 "cat /sys/class/net/%s/speed 2>/dev/null | awk '{print $1\" Mbps\"}'",
                 iface);
        show("Link Speed", cmd);

        snprintf(cmd, sizeof(cmd),
                 "cat /sys/class/net/%s/duplex 2>/dev/null",
                 iface);
        show("Duplex", cmd);

        snprintf(cmd, sizeof(cmd),
                 "cat /sys/class/net/%s/mtu 2>/dev/null",
                 iface);
        show("MTU", cmd);

        snprintf(cmd, sizeof(cmd),
                 "cat /sys/class/net/%s/operstate 2>/dev/null",
                 iface);
        show("State", cmd);

        show("Driver",
             "lspci -v 2>/dev/null | grep -A 1 -i ethernet | grep -i 'kernel driver' | awk '{print $NF}' | head -1");
    } else {
        label("Connection");  printf("(unknown)\n");
    }

    printf("  ----------------------------------------\n");
    show("Default Route", "ip route | grep default | awk '{print $3}'");
    show("DNS", "cat /etc/resolv.conf | grep nameserver | awk '{print $2}'");
}

int main(int argc, char *argv[]) {
    int show_password = 0;
    int show_speed    = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: %s [OPTIONS]\n\n", argv[0]);
            printf("  -p, --password   Show WiFi password (needs admin/sudo)\n");
            printf("  -s, --speed      Run internet speed test (needs curl)\n");
            printf("  -h, --help       Show this help\n");
            return 0;
        }
        if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--password"))
            show_password = 1;
        if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--speed"))
            show_speed = 1;
    }

    const char *platform = "Unknown";
    if (IsWindows())      platform = "Windows";
    else if (IsLinux())   platform = "Linux";
    else if (IsXnu())     platform = "macOS";

    printf("\n");
    printf("  Platform              %s\n", platform);
    printf("  ----------------------------------------\n");

    if (IsWindows()) {
        show_windows(show_password);
    } else if (IsLinux()) {
        show_linux(show_password, argv[0]);
    } else if (IsXnu()) {
        show("IPv4 Address", "ifconfig | grep 'inet ' | grep -v 127.0.0.1 | awk '{print $2}'");
        show("Hardware Address", "ifconfig | grep ether | awk '{print toupper($2)}'");
    }

    if (show_speed) {
        show_speedtest();
    }

    printf("\n");
    return 0;
}