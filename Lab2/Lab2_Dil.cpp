#include <iostream>
#include <vector>
#include <string>
#include "pidUtil.h"

using namespace std;

int main() {
    vector<int> pids;
    ErrStatus err = GetAllPids(pids);

    if (err == 0) {
        for (size_t i = 0; i < pids.size(); ++i) {
            int pid = pids[i];
            string name;
            err = GetNameByPid(pid, name);
            if (err == 0) {
                cout << "PID: " << pid << ", Name: " << name << endl;
            } else {
                cout << "Error: " << GetErrorMsg(err) << endl;
            }
        }
    } else {
        cout << "Error getting PIDs: " << GetErrorMsg(err) << endl;
    }

    return 0;
}
