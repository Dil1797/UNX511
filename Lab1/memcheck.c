#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "process_monitor.h"

#define STATUS_PATH_LEN 256
#define LINE_LEN 256
#define RSS_THRESHOLD 10000  // 10 MB

// Function to check if a string is all digits (PID)
int is_number(const char *str) {
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

int main() {
    DIR *proc = opendir("/proc");
    struct dirent *entry;

    if (proc == NULL) {
        perror("opendir /proc");
        return 1;
    }

    while ((entry = readdir(proc)) != NULL) {
        if (!is_number(entry->d_name))
            continue;

        char status_path[STATUS_PATH_LEN];
        snprintf(status_path, sizeof(status_path), "/proc/%s/status", entry->d_name);

        FILE *status = fopen(status_path, "r");
        if (!status) continue;

        char line[LINE_LEN];
        char name[LINE_LEN] = "";
        int rss = 0;

        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "Name:", 5) == 0) {
                sscanf(line + 5, "%s", name);
            }
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line + 6, "%d", &rss);
                break;  // Once we get VmRSS, no need to keep reading
            }
        }

        fclose(status);

        if (rss > RSS_THRESHOLD) {
            printf("PID: %s | Name: %s | VmRSS: %d kB\n", entry->d_name, name, rss);
        }
    }

    closedir(proc);
    return 0;
}
