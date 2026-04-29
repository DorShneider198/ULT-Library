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
    //how many times thread was selected by scheduler,increasing in each context switich to this thread
    int quantums_run;  
    int sleep_quantums_left; 
    bool is_blocked; //is called on by uthread_block
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

    int quantum_usecs; //time of a single quantum, how long each thread gets the CPU for
    int total_quantums; //counter for total quntums of the program
    int running_thread_id;
    /**
     * when thread self terminates, it cannot free its own stack
     * so we mark the id of a self terminating thread to delete later
     */
    int thread_to_terminate;
    void update_sleeping_threads();

public:
    
    ThreadManager(int quantum_usecs);
    //since every array cell is a pointer, we need a to free each of the cells memory as well
    ~ThreadManager();
    //RAII, make sure copy ctr and assignment are not allowed, we want a single copy of ThreadManger
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;
    void init_main_thread();
    int get_running_thread_id() const;
    int spawn_thread(thread_entry_point entry_point);
    int terminate_thread(int tid);
    void context_switch();    
    int get_total_quantums() const;
    int get_thread_quantums(int tid) const;
    void terminate_current_and_switch();
    int block_thread(int tid);
    int resume_thread(int tid);
    void sleep_current_thread(int quantums);
};

#endif // THREAD_MANAGER_H