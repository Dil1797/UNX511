//intfMonitor_solution.cpp - An interface monitor
//
// 13-Jul-20  M. Watler         Created.

#include <fcntl.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

const int MAXBUF=128;
bool isRunning=false;

//TODO 1: signal handler function prototype.This function will handle signals sent to this process.
void signalHandler(int signalNum);


int main(int argc, char *argv[])
{
    //TODO 2: Declare a variable of type struct sigaction .   
    //      For sigaction, see http://man7.org/linux/man-pages/man2/sigaction.2.html
    
    struct sigaction sa;  // Declare a variable of type struct sigaction
    sa.sa_handler = signalHandler;  // Assign the signal handler function
    sigemptyset(&sa.sa_mask);  // Clear any signals that should be blocked while handling
    sa.sa_flags = 0;  // Use default behavior

    char interface[MAXBUF];
    char statPath[MAXBUF];
    const char logfile[]="Network.log";//store network data in Network.log
    int retVal=0;

    //TODO 3: Register signal handlers for SIGUSR1, SIGUSR2, ctrl-C and ctrl-Z
    //TODO: Ensure there are no errors in registering the handlers
    
    // Register signal handlers for SIGUSR1, SIGUSR2, Ctrl-C (SIGINT), and Ctrl-Z (SIGTSTP)
    if (sigaction(SIGUSR1, &sa, NULL) == -1 ||
        sigaction(SIGUSR2, &sa, NULL) == -1 ||
        sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTSTP, &sa, NULL) == -1) {
        cerr << "Error registering signal handlers" << endl;
        return 1;
       }

    
    

    strncpy(interface, argv[1], MAXBUF);//The interface has been passed as an argument to intfMonitor
    int fd=open(logfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    cout<<"intfMonitor:main: interface:"<<interface<<":  pid:"<<getpid()<<endl;

    //TODO 4 : Wait for SIGUSR1 - the start signal from the parent
      // The child process should not start monitoring until the parent sends SIGUSR1.
    
     while (!isRunning) {
        sleep(1);
    }
    
    

    while(isRunning) {
        //Gather some network statistics and write to log file
        int tx_bytes=0;
        int rx_bytes=0;
        int tx_packets=0;
        int rx_packets=0;
	    ifstream infile;
            sprintf(statPath, "/sys/class/net/%s/statistics/tx_bytes", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>tx_bytes;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/rx_bytes", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>rx_bytes;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/tx_packets", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>tx_packets;
	        infile.close();
	    }
            sprintf(statPath, "/sys/class/net/%s/statistics/rx_packets", interface);
	    infile.open(statPath);
	    if(infile.is_open()) {
	        infile>>rx_packets;
	        infile.close();
	    }
	    char data[MAXBUF];
	    //write the stats into Network.log
	    int len=sprintf(data, "%s: tx_bytes:%d rx_bytes:%d tx_packets:%d rx_packets: %d\n", interface, tx_bytes, rx_bytes, tx_packets, rx_packets);
	    write(fd, data, len);
	    sleep(1);
    }
    close(fd);

    return 0;
}

     //TODO 6: Create a signal handler that starts your program on SIGUSR1 (sets isRunning to true),
//      stops your program on SIGUSR2 (sets isRunning to false),
//      and discards any ctrl-C or ctrl-Z.
//
//      If the signal handler receives a SIGUSR1, the following message should appear on the screen:
//      intfMonitor: starting up
//
//      If the signal handler receives a ctrl-C, the following message should appear on the screen:
//      intfMonitor: ctrl-C discarded
//
//      If the signal handler receives a ctrl-Z, the following message should appear on the screen:
//      intfMonitor: ctrl-Z discarded
//
//      If the signal handler receives a SIGUSR2, the following message should appear on the screen:
//      intfMonitor: shutting down
//
//      If the signal handler receives any other signal, the following message should appear on the screen:
//      intfMonitor: undefined signal

     void signalHandler(int signalNum) {
    switch (signalNum) {
        case SIGUSR1:  // Start monitoring
            cout << "intfMonitor: starting up" << endl;
            isRunning = true;
            break;
        case SIGUSR2:  // Stop monitoring
            cout << "intfMonitor: shutting down" << endl;
            isRunning = false;
            break;
        case SIGINT:  // Ctrl+C should be ignored
            cout << "intfMonitor: ctrl-C discarded" << endl;
            break;
        case SIGTSTP:  // Ctrl+Z should be ignored
            cout << "intfMonitor: ctrl-Z discarded" << endl;
            break;
        default:    // Any other signal
            cout << "intfMonitor: undefined signal" << endl;
    }
}



