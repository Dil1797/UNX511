    #ifndef PROCESS_MONITOR_H_  // Include guard to prevent multiple inclusions
    #define PROCESS_MONITOR_H_

    #define STATUS_PATH_LEN 256
    #define LINE_LEN 256
    #define RSS_THRESHOLD 10000  // 10 MB

    int is_number(const char *str);

    #endif
