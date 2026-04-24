#include "uthreads.h"
#include <iostream>
#include <cstdlib>

void thread2_entry() {
    std::cout << "Thread 2 started. Quanta: " << uthread_get_quantums(2) << " (Expected: 1)" << std::endl;
    std::cout << "Thread 2 yielding..." << std::endl;
    uthread_sleep(0);
    
    // We should never reach this line because Thread 1 will terminate Thread 2!
    std::cout << "ERROR: Thread 2 should have been terminated by Thread 1!" << std::endl;
    exit(1);
}

void thread1_entry() {
    std::cout << "Thread 1 started. Quanta: " << uthread_get_quantums(1) << " (Expected: 1)" << std::endl;
    std::cout << "Thread 1 yielding..." << std::endl;
    uthread_sleep(0);

    // Now it's Thread 1's second turn
    std::cout << "Thread 1 resumed. Quanta: " << uthread_get_quantums(1) << " (Expected: 2)" << std::endl;
    std::cout << "Thread 1 terminating Thread 2 (External termination)..." << std::endl;
    
    if (uthread_terminate(2) == -1) {
        std::cout << "ERROR: Failed to terminate Thread 2!" << std::endl;
        exit(1);
    }

    std::cout << "Thread 1 self-terminating..." << std::endl;
    uthread_terminate(1); // Self termination
    
    // We should never reach this line!
    std::cout << "ERROR: Thread 1 should be dead!" << std::endl;
    exit(1);
}

int main() {
    std::cout << "Initializing library..." << std::endl;
    uthread_init(100000);

    std::cout << "Main thread quanta: " << uthread_get_quantums(0) << " (Expected: 1)" << std::endl;
    std::cout << "Total quanta: " << uthread_get_total_quantums() << " (Expected: 1)" << std::endl;

    uthread_spawn(thread1_entry); // TID 1
    uthread_spawn(thread2_entry); // TID 2

    std::cout << "Main thread yielding..." << std::endl;
    uthread_sleep(0);

    // After Thread 1 and 2 run their first quanta, Main thread is back.
    // Quanta count: Main(1) -> T1(1) -> T2(1) -> Main(2). Total should be 4.
    std::cout << "Main thread resumed. Total quanta: " << uthread_get_total_quantums() << " (Expected: 4)" << std::endl;

    std::cout << "Main thread yielding again..." << std::endl;
    uthread_sleep(0);

    // T1 runs again, terminates T2, then self-terminates.
    // Next in line is Main Thread again. (Total quanta should now be 6).
    std::cout << "Main thread back. Total quanta: " << uthread_get_total_quantums() << " (Expected: 6)" << std::endl;
    
    std::cout << "Verifying T1 and T2 are dead..." << std::endl;
    if (uthread_terminate(1) != -1) {
         std::cout << "ERROR: Thread 1 should be dead but uthread_terminate didn't return -1!" << std::endl;
         exit(1);
    }
    if (uthread_terminate(2) != -1) {
         std::cout << "ERROR: Thread 2 should be dead but uthread_terminate didn't return -1!" << std::endl;
         exit(1);
    }

    std::cout << "SUCCESS: test_3_5 passed flawlessly!" << std::endl;
    uthread_terminate(0);
    return 0;
}