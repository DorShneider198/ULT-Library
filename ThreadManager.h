#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "uthreads.h"
#include <queue>
#include <vector>
#include <setjmp.h>
#include <signal.h>
#include <list> 


enum ThreadState {
    RUNNING,
    READY,
    BLOCKED
};

struct TCB {
    int id;
    ThreadState state;
    int quantums_run;  //how many times thread was selected by scheduler 
    int sleep_quantums_left; 
    char* stack;   
    //env is a field to store CPU registers status such as PC,SP
    sigjmp_buf env;
};
/**
 * Threads will be managed in an array(and not unordered map since the thread's number is bounded)
 * where the index is the thread id and each cell contain a pointer to current thread's control block.
 */
class ThreadManager {
private:
    TCB* threads[MAX_THREAD_NUM];
    
    std::list<int> ready_queue;
    
    // min heap to find next available id(which is the smallest)
    std::priority_queue<int, std::vector<int>, std::greater<int>> available_ids;

    int quantum_usecs; //time of a single quantum
    int total_quantums; //counter for total quntums of the program
    int running_thread_id;

public:
    
    ThreadManager(int quantum_usecs);
    //since every array cell is a pointer, we need a to free each of the cells memory as well
    ~ThreadManager();
    //RAII, make sure copy constructor and assignment are not allowed, we want a single copy of ThreadManger
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;
    void init_main_thread();
    int get_running_thread_id() const;
    int spawn_thread(thread_entry_point entry_point);
    int terminate_thread(int tid);
    
};

#endif // THREAD_MANAGER_H