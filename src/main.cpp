
#include "./cpu/CONTROL_UNIT.h"
#include "./memory/MAINMEMORY.h"
#include "./loader/loader.h"
#include "./assembler/assembler.h"
#include "./PCB.h"

#include <unistd.h>
#include <cstdlib>
#include <pthread.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>

using namespace std;

#define NUM_CORES 2
#define QUANTUM 10 
// 1 clock = 1 quantum

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

void* coreManage(void* arg) {
    scheduleInfo* info = static_cast<scheduleInfo*>(arg);

    while (!info->shutdown) {
        PCB* currentProcess = nullptr;

        {
            lock_guard<mutex> lock(*info->queueLock);

            
            // Find a ready process
            for (auto& pcb : *info->processes) {
                if (pcb->state == State::Ready && pcb->state != State::Blocked) {
                    currentProcess = pcb.get();
                    currentProcess->state = State::Executing;
                    break;
                }
            }
        }

        if (currentProcess) {
            Core(*info->ram, *currentProcess, info->ioRequests,info->printLock);

            lock_guard<mutex> lock(*info->queueLock);
            if (currentProcess->state == State::Executing) {
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

        if(!info->ioRequests->empty()){
            allDone = false;
        }
                
        if (allDone) {
            //cout << "Shutdown" << endl;
            info->shutdown = true;
        }
    }

    return nullptr;
}


void * resourceManager(void * arg){
    scheduleInfo* info = static_cast<scheduleInfo*>(arg);
    
   
    while (!info->shutdown) {

        if(!info->ioRequests->empty()){
            
            auto req = move(info->ioRequests->front());
            info->printLock = true;

            info->ioRequests->erase(info->ioRequests->begin());

            // Simulate a 0.1 ms delay
            usleep(100000);

            cout << "Program " << req->process->id << ": " << req->msg << std::endl;          
             
            req->process->state = State::Ready;
            info->printLock = false;
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
    scheduleInfo->ioRequests = new vector<unique_ptr<ioRequest>>();
    scheduleInfo->queueLock = new mutex();
    scheduleInfo->shutdown = false;
    scheduleInfo->printLock = false;

    for (int i = 1; i < argc; i++) {
        auto pcb = make_unique<PCB>();
        pcb->baseAddr = initProgram[i - 1];
        pcb->finalAddr = initProgram[i];
        pcb->quantum = QUANTUM;
        pcb->id = i;
        pcb->state = State::Ready;
        pcb->regBank.pc.value = initProgram[i - 1];
        scheduleInfo->processes->push_back(move(pcb));
    }

    pthread_t threads[NUM_CORES + 2];

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
    if (pthread_create(&threads[NUM_CORES +1], nullptr, resourceManager, scheduleInfo.get()) != 0) {
        cerr << "Error creating I/O manage thread" << endl;
        return 1;
    }

    // Join threads
    for (int i = 0; i < NUM_CORES + 2; i++) {
        pthread_join(threads[i], nullptr);
    }

    return 0;
}
