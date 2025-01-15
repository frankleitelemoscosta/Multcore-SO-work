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

enum class Scalling{
    FCFS = 1,
    Priority = 2
};

struct PCB{
  unsigned int quantum;
  REGISTER_BANK regBank;
  State state;
  int baseAddr;
  int finalAddr;
  int id;
  Scalling scaling;
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
};

#endif
