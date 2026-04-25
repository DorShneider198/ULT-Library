#include "uthreads.h"
#include "ThreadManager.h"
#include <iostream>
#include <sys/time.h>
#include <signal.h>
//creating a single manager object
static ThreadManager* manager = nullptr;
// a strcture to hold signals
static sigset_t timer_set;
/**
 * take signals stored in timer_set and block them until further notice
 */
void block_timer() {
    if (sigprocmask(SIG_BLOCK, &timer_set, NULL) < 0) {
        std::cerr << "system error: sigprocmask block failed\n";
        exit(1);
    }
}

/**
 *  unblock the signals inside timer_set, if a timer has finished while being blocked, the signal will
    be passed to the thread right after reading this function
 */
void unblock_timer() {
    if (sigprocmask(SIG_UNBLOCK, &timer_set, NULL) < 0) {
        std::cerr << "system error: sigprocmask unblock failed\n";
        exit(1);
    }
}

void timer_handler(int sig) {
    if (manager != nullptr) {
        manager->context_switch();
    }
}
/**
 * @brief initializes the thread library.
 *
 * Once this function returns, the main thread (tid == 0) will be set as RUNNING. There is no need to 
 * provide an entry_point or to create a stack for the main thread - it will be using the 
 * "regular" stack and PC.
 * You may assume that this function is called before any other thread library function, 
 * and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs <= 0) {
        std::cerr << "thread library error: " << "Illegal quantum length" << std::endl;
        return -1;
    }
    
    if (manager != nullptr) {
        std::cerr << "thread library error: Library already initialized" << std::endl;
        return -1;
    }
    manager = new ThreadManager(quantum_usecs);
    manager->init_main_thread();
    /* clean timer_set from garbage values and put the SIGVTALRM inside the set (so block_timer will know
    which signal to block, the SIGVTALRM */
    if (sigemptyset(&timer_set) < 0 || sigaddset(&timer_set, SIGVTALRM) < 0) {
            std::cerr << "system error: sigset setup failed\n";
            exit(1);
        }

    struct sigaction sa = {0};
    //when signal is coming, go run timer_handler(context_switch)
    sa.sa_handler = &timer_handler;
    //specify for which signal we want to run the handler( in this case SIGVTALRM)
    if (sigaction(SIGVTALRM, &sa, NULL) < 0) {
        std::cerr << "system error: sigaction error\n";
        exit(1);
    }
    struct itimerval timer;
    timer.it_value.tv_sec = quantum_usecs / 1000000;
    timer.it_value.tv_usec = quantum_usecs % 1000000;
    timer.it_interval.tv_sec = quantum_usecs / 1000000;
    timer.it_interval.tv_usec = quantum_usecs % 1000000;

    //start the timer only when a thread is running on the cpu
    if (setitimer(ITIMER_VIRTUAL, &timer, NULL) < 0) {
        std::cerr << "system error: setitimer error\n";
        exit(1);
    }
    return 0;
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 * It is an error to call this function with a null entry_point.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point) {
    block_timer();
    if (manager == nullptr) {
            std::cerr << "thread library error: Library not initialized" << std::endl;
            unblock_timer();
            return -1;
    }
    int res = manager->spawn_thread(entry_point);
    
    unblock_timer();
    return res;
}



/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread 
 * with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the 
 * termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. 
 * If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid){
    block_timer();
    if (manager == nullptr) {
        unblock_timer();
        return -1;
    }    
    if (tid == 0) {
        delete manager; 
        exit(0);
    }
    //case where a thread tries to terminate itself
    if (tid == manager->get_running_thread_id()) {
        manager->terminate_current_and_switch();
        return 0;
    }
    int res = manager->terminate_thread(tid);
    
    unblock_timer();
    return res;
}


/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is 
 * an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should 
 * be made. Blocking a thread in
 * BLOCKED state has no effect and is *not* considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    block_timer();
    
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized\n";
        unblock_timer();
        return -1;
    }
    
    int res = manager->block_thread(tid);
    
    unblock_timer();
    return res;
}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as 
 * an error. If no thread with
 * ID tid exists it is considered an error.
 * When a thread transition to the READY state it is placed at the end of the READY queue.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    block_timer();
    
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized\n";
        unblock_timer();
        return -1;
    }
    
    int res = manager->resume_thread(tid);
    
    unblock_timer();
    return res;
}

/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling 
 * decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY queue.
 * If the thread which was just RUNNING should also be added to the READY queue, or if 
 * multiple threads wake up 
 * at the same time, the order in which they're added to the end of the READY queue doesn't matter.
 * The number of quantums refers to the number of times a new quantum starts, regardless of 
 * the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * A call with num_quantums == 0 will immediately stop the thread and move it to 
 * the back of the execution queue.
 * 
 * It is considered an error if the main thread (tid == 0) calls this function with 
 * num_quantums != 0.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums) {
    block_timer();
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized" << std::endl;
        unblock_timer();
        return -1;
    }
    
    if (num_quantums < 0) {
        std::cerr << "thread library error: num_quantums must be non-negative" << std::endl;
        unblock_timer();
        return -1;
    }

    if (manager->get_running_thread_id() == 0 && num_quantums != 0) {
        std::cerr << "thread library error: main thread cannot sleep" << std::endl;
        unblock_timer();
        return -1;
    }

if (num_quantums == 0) {
        manager->context_switch();
    } else { //quntum >0
        manager->sleep_current_thread(num_quantums);
    }
    
    unblock_timer();
    return 0;
}

/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid() {
    block_timer();
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized" << std::endl;
        unblock_timer();
        return -1;
    }
    unblock_timer();
    return manager->get_running_thread_id();
}

/**
 * @brief Returns the total number of quantums since the library was initialized, 
 * including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should 
 * be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums() {
    block_timer();
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized" << std::endl;
        unblock_timer();
        return -1;
    }
    unblock_timer();
    return manager->get_total_quantums();
}


/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum 
 * that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this 
 * function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    block_timer();
    if (manager == nullptr) {
        std::cerr << "thread library error: Library not initialized" << std::endl;
        unblock_timer();
        return -1;
    }
    unblock_timer();
    return manager->get_thread_quantums(tid);
}
