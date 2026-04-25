#include "uthreads.h"
#include <iostream>
#include <cstdlib>

bool t1_awoke = false;

void thread1_entry() {
    std::cout << "T1: Going to sleep for 5 quantums." << std::endl;
    if (uthread_sleep(5) == -1) exit(1);
    
    // לשורה הזו מותר להגיע רק אחרי שהזמן עבר *וגם* מישהו עשה לו Resume
    std::cout << "T1: Awoke!" << std::endl;
    t1_awoke = true;
    uthread_terminate(uthread_get_tid());
}

void thread2_entry() {
    std::cout << "T2: Running. Blocking T1 while it is sleeping!" << std::endl;
    if (uthread_block(1) == -1) exit(1);
    std::cout << "T2: Terminating self." << std::endl;
    uthread_terminate(uthread_get_tid());
}
void thread3_entry() {
    std::cout << "T3: Going to sleep for 3 quantums." << std::endl;
    if (uthread_sleep(3) == -1) exit(1);
    std::cout << "T3: Awoke simultaneously with T4!" << std::endl;
    
    // מחיקה עצמית בטוחה!
    uthread_terminate(uthread_get_tid()); 
}

void thread4_entry() {
    std::cout << "T4: Going to sleep for 2 quantums." << std::endl;
    if (uthread_sleep(2) == -1) exit(1);
    std::cout << "T4: Awoke simultaneously with T3!" << std::endl;
    
    // מחיקה עצמית בטוחה!
    uthread_terminate(uthread_get_tid()); 
}

int main() {
    std::cout << "Initializing library..." << std::endl;
    uthread_init(100000);

    // --- מקרה קצה 1: חסימה כפולה ---
    std::cout << "\n--- Edge Case 1: The Double Block (Sleep + Manual Block) ---" << std::endl;
    uthread_spawn(thread1_entry); // TID 1
    uthread_spawn(thread2_entry); // TID 2

    uthread_sleep(0); // החוט הראשי מוותר על המעבד כדי ש-T1 ירוץ

    // בשלב זה T1 הלך לישון, ו-T2 חסם אותו ונמחק. חזרנו לחוט הראשי.
    std::cout << "Main: T1 is now sleeping AND manually blocked. Advancing time..." << std::endl;
    
    // מריצים 8 קוונטומים (מעבר ל-5 ש-T1 ביקש לישון)
    for (int i = 0; i < 8; i++) {
        uthread_sleep(0); 
    }

    if (t1_awoke) {
        std::cout << "ERROR: T1 awoke before being resumed! The boolean flag failed!" << std::endl;
        exit(1);
    } else {
        std::cout << "SUCCESS: T1 did not wake up from sleep alone because it was blocked." << std::endl;
    }

    std::cout << "Main: Resuming T1..." << std::endl;
    if (uthread_resume(1) == -1) exit(1);

    uthread_sleep(0); // מוותרים על המעבד שוב כדי לתת ל-T1 לרוץ סוף סוף

    if (!t1_awoke) {
        std::cout << "ERROR: T1 did not wake up after being resumed!" << std::endl;
        exit(1);
    }

    // --- מקרה קצה 2: התעוררות בו זמנית ---
    std::cout << "\n--- Edge Case 2: Multiple Threads Waking Up Simultaneously ---" << std::endl;
    uthread_spawn(thread3_entry); // TID 3
    uthread_spawn(thread4_entry); // TID 4

    uthread_sleep(0); // מוותרים לטובת T3 שהולך לישון, ואז T4 הולך לישון

    // מקדמים את הזמן. שניהם יתעוררו במדויק באותו קוונטום
    for (int i = 0; i < 5; i++) {
        uthread_sleep(0); 
    }

    std::cout << "\nSUCCESS: test_5_sleep_edge_cases passed!" << std::endl;
    uthread_terminate(0);
    return 0;
}