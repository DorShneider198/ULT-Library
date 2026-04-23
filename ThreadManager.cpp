#include "ThreadManager.h"
#include <iostream>
typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

ThreadManager::ThreadManager(int quantum_usecs) 
    : quantum_usecs(quantum_usecs), total_quantums(0), running_thread_id(-1) 
{
    
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        threads[i] = nullptr;
    }

    // id=0 resrved for main Thread.
    for (int i = 1; i < MAX_THREAD_NUM; ++i) {
        available_ids.push(i);
    }
}


ThreadManager::~ThreadManager() {
    for (int i = 0; i < MAX_THREAD_NUM; ++i) {
        if (threads[i] != nullptr) {
            //release first the pointer's memory
            if (i != 0 && threads[i]->stack != nullptr) {
                delete[] threads[i]->stack;
            }
            delete threads[i];
        }
    }
}

//init main theread
void ThreadManager::init_main_thread() {
    TCB* main_thread = new TCB();
    main_thread->id = 0;
    main_thread->state = RUNNING;
    main_thread->quantums_run = 1;
    main_thread->sleep_quantums_left = 0;
    //no need to allocated stack for main thread in ex instructions
    main_thread->stack = nullptr; 
    threads[0] = main_thread;
    running_thread_id = 0;
    total_quantums++;
}
int ThreadManager::get_running_thread_id() const {
    return running_thread_id;
}

int ThreadManager::spawn_thread(thread_entry_point entry_point) {
    if (entry_point == nullptr) {
        std::cerr << "thread library error: entry_point cannot be null" << std::endl;
        return -1;
    }
    
    if (available_ids.empty()) {
        std::cerr << "thread library error: reached maximum number of threads" << std::endl;
        return -1;
    }
    //available id implemented as min heap,newly spawn thread get min free id.
    int new_id = available_ids.top();
    available_ids.pop();
    //allocate thread struct fields.
    TCB* new_thread = new TCB();
    new_thread->id = new_id;
    new_thread->state = READY;
    new_thread->quantums_run = 0;
    new_thread->sleep_quantums_left = 0;
    new_thread->stack = new char[STACK_SIZE];

    //setup Context:
    //note to myself: address_t is just a convinient type to work with memory
    address_t sp = (address_t)new_thread->stack + STACK_SIZE - sizeof(address_t);
    //entry_point is the value of the Program Counter from which the created thread begins its code?
    address_t pc = (address_t)entry_point;
    //create a "snapshot" of CPU registers and store it in env
    sigsetjmp(new_thread->env, 1);
    //override garbage values with the actual thread's relevant data.
    (new_thread->env->__jmpbuf)[JB_SP] = translate_address(sp);
    (new_thread->env->__jmpbuf)[JB_PC] = translate_address(pc);
    
    //reset state of signals
    sigemptyset(&new_thread->env->__saved_mask);

    threads[new_id] = new_thread;
    ready_queue.push(new_id);
    return new_id;
}