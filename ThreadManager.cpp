#include "ThreadManager.h"
#include <iostream>
typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
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
    main_thread->is_blocked = false;
    //no need to allocated stack for main thread in ex instructions
    main_thread->stack = nullptr; 
    threads[0] = main_thread;
    running_thread_id = 0;
    total_quantums++;
}
int ThreadManager::get_running_thread_id() const {
    return running_thread_id;
}
int ThreadManager::get_total_quantums() const {
    return total_quantums;
}
int ThreadManager::get_thread_quantums(int tid) const {
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: thread " << tid << " does not exist" << std::endl;
        return -1;
    }
    
    return threads[tid]->quantums_run;
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
    new_thread->is_blocked = false;
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
    ready_queue.push_back(new_id);
    return new_id;
}

//this function has 2 steps: realease the memory and return the id to the free id heap
int ThreadManager::terminate_thread(int tid) {

    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: thread " << tid << " does not exist" << std::endl;
        return -1;
    }

    if(tid == 0) {
        return 0;
    }
    //removing thread from the running ready queue
    ready_queue.remove(tid);
    //free memory

    if (threads[tid]->stack != nullptr) {
        delete[] threads[tid]->stack;
    }
    delete threads[tid];
    threads[tid] = nullptr;

    //recycyle id back to avilavbes
    available_ids.push(tid);

    return 0;

}
/**
 * current thread gives up CPU, changes state to READY and goes to end of queue.
 */
void ThreadManager::context_switch() {
    int current_tid = running_thread_id;
    //save State of currently running thread
    int ret_val = sigsetjmp(threads[current_tid]->env, 1);

    if (ret_val == 0) {

        if (threads[current_tid]->state == RUNNING) {
            threads[current_tid]->state = READY;
            ready_queue.push_back(current_tid);
        } 
        //iterate all other threads and -- their sleep_left since context switching "starting a new run on CPU"
        for (int i = 1; i < MAX_THREAD_NUM; ++i) { 
            if (threads[i] != nullptr && threads[i]->sleep_quantums_left > 0) {
                
                threads[i]->sleep_quantums_left--;
                //if a thread done sleeping push it back to ready queue
                //if done sleeping AND no other thread is blocking it 
                if (threads[i]->sleep_quantums_left == 0 && !threads[i]->is_blocked) {
                    threads[i]->state = READY;
                    ready_queue.push_back(i);
                }
            }
        }
        
        int next_tid = ready_queue.front();
        ready_queue.pop_front();
        
        running_thread_id = next_tid;
        threads[next_tid]->state = RUNNING;
        
        total_quantums++;
        threads[next_tid]->quantums_run++;
        //tell CPU to jump to "snapshot" of the next thread
        siglongjmp(threads[next_tid]->env, 1);
    }  
}
void ThreadManager::terminate_current_and_switch() {
    int tid = running_thread_id;
    
    if (threads[tid]->stack != nullptr) {
        delete[] threads[tid]->stack;
    }
    delete threads[tid];
    threads[tid] = nullptr;
    available_ids.push(tid);
    
    int next_tid = ready_queue.front();
    ready_queue.pop_front();
    
    running_thread_id = next_tid;
    threads[next_tid]->state = RUNNING;
    
    total_quantums++;
    threads[next_tid]->quantums_run++;
    
    siglongjmp(threads[next_tid]->env, 1);
}
int ThreadManager::block_thread(int tid) {
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: thread " << tid << " does not exist" << std::endl;
        return -1;
    }
    if (tid == 0) {
        std::cerr << "thread library error: cannot block main thread" << std::endl;
        return -1;
    }

    threads[tid]->is_blocked = true;

    if (threads[tid]->state != BLOCKED) {
        threads[tid]->state = BLOCKED;
        //if a thread blocks itself, it need to clear the CPU and switch to next thread in queue
        if (tid == running_thread_id) {
            context_switch(); 
        } else {
            ready_queue.remove(tid);
        }
    }
    
    return 0;
}

int ThreadManager::resume_thread(int tid) {
    if (tid < 0 || tid >= MAX_THREAD_NUM || threads[tid] == nullptr) {
        std::cerr << "thread library error: thread " << tid << " does not exist" << std::endl;
        return -1;
    }
    
    threads[tid]->is_blocked = false;
    
    if (threads[tid]->state == BLOCKED && threads[tid]->sleep_quantums_left == 0) {
        threads[tid]->state = READY;
        ready_queue.push_back(tid);
    }
    
    return 0;
}
void ThreadManager::sleep_current_thread(int quantums) {
    int tid = running_thread_id;
    threads[tid]->sleep_quantums_left = quantums + 1;
    threads[tid]->state = BLOCKED;
    
    context_switch();
}