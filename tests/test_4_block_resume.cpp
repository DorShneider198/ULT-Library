#include "uthreads.h"
#include <iostream>
#include <cstdlib>

void thread1_entry() {
    std::cout << "T1: Running. Blocking T2..." << std::endl;
    if (uthread_block(2) == -1) {
        std::cout << "ERROR: Failed to block T2!" << std::endl;
        exit(1);
    }
    std::cout << "T1: Yielding..." << std::endl;
    // T1 מוותר על המעבד. מכיוון ש-T2 חסום, המעבד יחזור לחוט הראשי (T0)
    uthread_sleep(0);

    std::cout << "T1: Running again. Terminating self." << std::endl;
    uthread_terminate(1);
}

void thread2_entry() {
    // השורה הזו תודפס רק אחרי שהחוט הראשי ישחרר את T2
    std::cout << "T2: Running. This should only happen AFTER Main resumes T2." << std::endl;
    std::cout << "T2: Terminating self." << std::endl;
    uthread_terminate(2);
}

void thread3_entry() {
    std::cout << "ERROR: T3 should never run because it will be terminated while blocked!" << std::endl;
    exit(1);
}

int main() {
    std::cout << "Initializing library..." << std::endl;
    uthread_init(100000);

    // --- Error Handling Tests ---
    std::cout << "\n--- Testing Error Handling ---" << std::endl;
    if (uthread_block(0) != -1) {
        std::cout << "ERROR: Blocked main thread!" << std::endl; exit(1);
    }
    if (uthread_block(99) != -1) {
        std::cout << "ERROR: Blocked non-existent thread!" << std::endl; exit(1);
    }
    if (uthread_resume(99) != -1) {
        std::cout << "ERROR: Resumed non-existent thread!" << std::endl; exit(1);
    }
    std::cout << "Error Handling passed." << std::endl;

    // --- Basic Block/Resume ---
    std::cout << "\n--- Testing Basic Block/Resume ---" << std::endl;
    uthread_spawn(thread1_entry); // TID 1
    uthread_spawn(thread2_entry); // TID 2

    std::cout << "Main: Yielding to T1..." << std::endl;
    uthread_sleep(0); 

    std::cout << "Main: Back in Main. Resuming T2..." << std::endl;
    if (uthread_resume(2) == -1) {
        std::cout << "ERROR: Failed to resume T2!" << std::endl; exit(1);
    }
    
    std::cout << "Main: Yielding to let T1 and T2 finish..." << std::endl;
    uthread_sleep(0); // T1 ירוץ ויסיים את עצמו, ואז T2 ירוץ ויסיים את עצמו

    // --- Edge Case: Terminate while Blocked ---
    std::cout << "\n--- Testing Edge Case: Terminate while blocked ---" << std::endl;
    int tid3 = uthread_spawn(thread3_entry); 
    std::cout << "Spawned T3 with TID: " << tid3 << std::endl;
    
    std::cout << "Blocking T3..." << std::endl;
    uthread_block(tid3);
    
    std::cout << "Terminating T3 while it is blocked..." << std::endl;
    uthread_terminate(tid3);
    
    int tid4 = uthread_spawn(thread2_entry); // יצירת חוט סתמי רק כדי לבדוק מזהה
    std::cout << "Spawned T4 with TID: " << tid4 << " (Expected: " << tid3 << ")" << std::endl;
    
    if (tid3 != tid4) {
        std::cout << "ERROR: TID was not recycled properly for a blocked thread!" << std::endl; 
        exit(1);
    }

    std::cout << "\nSUCCESS: test_4_block_resume passed flawlessly!" << std::endl;
    uthread_terminate(0);
    return 0;
}