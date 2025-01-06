
#include "./cpu/CONTROL_UNIT.h"
#include "./memory/MAINMEMORY.h"
#include "./loader/loader.h"
#include "./assembler/assembler.h"
#include "./PCB.h"

#include <cstdlib>
#include <pthread.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

using namespace std;

#define NUM_CORES 4
#define QUANTUM 10 # 1 clock = 1 quantum

string getFileName(char* file) {
    string filePath(file);
    filesystem::path path(filePath);
    string fileName = path.filename().string();
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos != string::npos) {
        fileName = fileName.substr(0, dotPos);
    }
    return fileName;
}

struct scheduleInfo {
    MainMemory* ram;
    vector<unique_ptr<PCB>>* processes;
    mutex* queueLock;
    mutex* printLock;
    bool shutdown;
};


void* coreManage(void* arg) {
    scheduleInfo* info = static_cast<scheduleInfo*>(arg);

    while (!info->shutdown) {
        PCB* currentProcess = nullptr;

        {
            lock_guard<mutex> lock(*info->queueLock);

            
            // Find a ready process
            for (auto& pcb : *info->processes) {
                if (pcb->state == State::Ready) {
                    currentProcess = pcb.get();
                    currentProcess->state = State::Executing;
                    break;
                }
            }
        }

        if (currentProcess) {
            Core(*info->ram, *currentProcess, info->printLock);

            lock_guard<mutex> lock(*info->queueLock);
            if (currentProcess->state != State::Finished) {
                currentProcess->state = State::Ready;
            }
        }
    }

    return nullptr;
}

void* scheduler(void* arg) {
    scheduleInfo* info = static_cast<scheduleInfo*>(arg);

    while (!info->shutdown) {
        lock_guard<mutex> lock(*info->queueLock);

        bool allDone = true;
        for (const auto& pcb : *info->processes) {
            if (pcb->state != State::Finished) {
                allDone = false;
                break;
            }
        }
        if (allDone) {
            info->shutdown = true;
        }
    }

    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_files>" << endl;
        return 1;
    }

    // Trim filepaths to get file names
    vector<string> files;
    for (int i = 0; i < argc; i++) {
        files.push_back(getFileName(argv[i]));
    }

    // Compile prograns
    assembleFiles(argc, files, argv);

    MainMemory ram = MainMemory(2048, 2048);
    int initProgram[argc];
    initProgram[0] = 0;
    
    for (int i = 1; i < argc; i++) {
        initProgram[i] = loadProgram(files[i], ram, initProgram[i - 1]);
    }

    auto scheduleInfo = make_unique<struct scheduleInfo>();
    scheduleInfo->ram = &ram;
    scheduleInfo->processes = new vector<unique_ptr<PCB>>();
    scheduleInfo->queueLock = new mutex();
    scheduleInfo->printLock = new mutex();
    scheduleInfo->shutdown = false;

    for (int i = 1; i < argc; i++) {
        auto pcb = make_unique<PCB>();
        pcb->baseAddr = initProgram[i - 1];
        pcb->finalAddr = initProgram[i];
        pcb->quantum = QUANTUM;
        pcb->id = i;
        pcb->state = State::Ready;
        scheduleInfo->processes->push_back(move(pcb));
    }

    pthread_t threads[NUM_CORES + 1];

    for (int i = 0; i < NUM_CORES; ++i) {
        if (pthread_create(&threads[i], nullptr, coreManage, scheduleInfo.get()) != 0) {
            cerr << "Error creating core thread " << i << endl;
            return 1;
        }
    }

    if (pthread_create(&threads[NUM_CORES], nullptr, scheduler, scheduleInfo.get()) != 0) {
        cerr << "Error creating scheduler thread" << endl;
        return 1;
    }

    // Join threads
    for (int i = 0; i < NUM_CORES + 1; i++) {
        pthread_join(threads[i], nullptr);
    }

    return 0;
}
