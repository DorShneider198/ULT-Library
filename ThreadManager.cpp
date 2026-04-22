#include "ThreadManager.h"
#include <iostream>

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