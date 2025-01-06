#include "./cpu/REGISTER_BANK.h"

#ifndef PCB_H
#define PCB_H

 
enum class State {
    Ready,
    Blocked,
    Executing,
    Finished
};

struct PCB{
  unsigned int quantum;
  REGISTER_BANK regBank;
  State state;
  int baseAddr;
  int finalAddr;
  int id;
};

#endif
