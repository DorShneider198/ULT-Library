#include "uthreads.h"
#include <iostream>

// פונקציה שעושה עבודה "שחורה" וטוחנת את המעבד
void heavy_computation() {
    int id = uthread_get_tid();
    std::cout << "Thread " << id << " started its heavy work..." << std::endl;
    
    // לולאה ארוכה מאוד בלי שום קריאה לספריית הת'רדים (אין sleep או block)
    // הדבר היחיד שיכול לקטוע אותה זה הטיימר של מערכת ההפעלה!
    long dummy_work = 0;
    for (long i = 0; i < 400000000; ++i) {
        dummy_work += 1; 
    }
    
    std::cout << "Thread " << id << " finished heavy work!" << std::endl;
    std::cout << "-> Thread " << id << " total quantums run: " << uthread_get_quantums(id) << std::endl;
    
    uthread_terminate(id);
}

int main() {
    std::cout << "--- Starting Preemption (TiK ToK) Test ---" << std::endl;
    
    // מאתחלים את הספרייה עם קוונטום של 100,000 מיקרו-שניות (עשירית שנייה)
    if (uthread_init(100000) == -1) {
        std::cerr << "Init failed" << std::endl;
        return 1;
    }

    std::cout << "Spawning Thread 1..." << std::endl;
    uthread_spawn(heavy_computation);

    std::cout << "Spawning Thread 2..." << std::endl;
    uthread_spawn(heavy_computation);

    std::cout << "Thread 0 (Main) starting its heavy work..." << std::endl;
    
    // גם ה-Main Thread טוחן את המעבד
    long dummy_work = 0;
    for (long i = 0; i < 400000000; ++i) {
        dummy_work += 1; 
    }
    
    std::cout << "Thread 0 (Main) finished heavy work!" << std::endl;
    std::cout << "-> Thread 0 total quantums run: " << uthread_get_quantums(0) << std::endl;
    std::cout << "-> Total quantums system wide: " << uthread_get_total_quantums() << std::endl;

    std::cout << "--- Test Finished Successfully ---" << std::endl;
    uthread_terminate(0);
    return 0;
}