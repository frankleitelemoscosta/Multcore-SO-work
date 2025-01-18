#include "./cpu/REGISTER_BANK.h"
#include "./memory/MAINMEMORY.h"
#include <mutex>
#include <memory>

#ifndef PCB_H
#define PCB_H

 
enum class State {
    Ready,
    Blocked,
    Executing,
    Finished
};

enum class Schedulling{
    FCFS = 1,
    Priority = 2,
    Random = 3,
    SJF = 4
};

struct PCB{
  int quantum;
  int timestamp;
  int size;
  REGISTER_BANK regBank;
  State state;
  int baseAddr;
  int finalAddr;
  int id;
  int priority;
};

struct ioRequest {
    string msg;
    PCB* process;
};

struct scheduleInfo {
    MainMemory* ram;
    vector<unique_ptr<PCB>>* processes;
    mutex* queueLock;
    vector<unique_ptr<ioRequest>>* ioRequests;
    bool shutdown;
    bool printLock;
    Schedulling schedulling;
};

#endif
