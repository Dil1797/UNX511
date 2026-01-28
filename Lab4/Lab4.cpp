#include <iostream>       // For input/output (cin, cout)
#include <fcntl.h>        // For open() and file operations
#include <sys/ioctl.h>    // For ioctl() function
#include <linux/fb.h>     // For framebuffer structures
#include <errno.h>        // For error handling using errno
#include <cstring>        // For strerror()
#include <unistd.h>       // For close() and dup2()

int main() {
    // Step 1: Redirect standard error to Screen.log (overwrite it each run)
    int log_fd = open("Screen.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (log_fd < 0) {
        std::cerr << "Error opening log file: " << strerror(errno) << std::endl;
        return 1;
    }
    dup2(log_fd, STDERR_FILENO);  // Redirect standard error to log file
    close(log_fd);                // We can close the original fd now

    // Step 2: Open the framebuffer device (read-only, non-blocking)
    int fb_fd = open("/dev/fb0", O_RDONLY | O_NONBLOCK);
    if (fb_fd < 0) {
        std::cerr << "Error opening framebuffer device: " << strerror(errno) << std::endl;
        return 1;
    }

    int choice;
    while (true) {
        std::cout << "\nSelect an option:\n";
        std::cout << "1. Fixed Screen Info\n";
        std::cout << "2. Variable Screen Info\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        if (choice == 0) break;

        if (choice == 1) {
            struct fb_fix_screeninfo fix_info;
            if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix_info) < 0) {
                std::cerr << "Error getting fixed screen info: " << strerror(errno) << std::endl;
            } else {
                std::cout << "\n--- Fixed Screen Info ---\n";
                std::cout << "ID: " << fix_info.id << std::endl;
                std::cout << "Visual Type: " << fix_info.visual << std::endl;
                std::cout << "Accelerator: " << fix_info.accel << std::endl;
                std::cout << "Capabilities: " << fix_info.capabilities << std::endl;

                /*
                Example constants for documentation:
                FB_VISUAL_TRUECOLOR = 2
                FB_ACCEL_NONE = 0
                FB_CAP_FOURCC = 1
                */
            }
        }

        if (choice == 2) {
            struct fb_var_screeninfo var_info;
            if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var_info) < 0) {
                std::cerr << "Error getting variable screen info: " << strerror(errno) << std::endl;
            } else {
                std::cout << "\n--- Variable Screen Info ---\n";
                std::cout << "X-Resolution: " << var_info.xres << std::endl;
                std::cout << "Y-Resolution: " << var_info.yres << std::endl;
                std::cout << "Bits per Pixel: " << var_info.bits_per_pixel << std::endl;

                /*
                Example values for documentation:
                xres = 1920
                yres = 1080
                bpp = 32
                */
            }
        }
    }

    close(fb_fd);  // Cleanup: close framebuffer device
    std::cout << "Program exited successfully." << std::endl;
    return 0;
}
