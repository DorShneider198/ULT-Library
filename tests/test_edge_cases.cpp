#include "uthreads.h"
#include <iostream>
#include <limits>

void do_nothing() {}

void test_invalid_init() {
    std::cout << "\n=== Test: Invalid Init ===" << std::endl;
    // negative quantum
    if (uthread_init(-1) != -1) { std::cout << "FAIL: should reject negative quantum" << std::endl; return; }
    // zero quantum
    if (uthread_init(0) != -1) { std::cout << "FAIL: should reject zero quantum" << std::endl; return; }
    std::cout << "PASS" << std::endl;
}

void test_terminate_nonexistent() {
    std::cout << "\n=== Test: Terminate Non-Existent Thread ===" << std::endl;
    // thread 50 was never created
    if (uthread_terminate(50) != -1) { std::cout << "FAIL: should return -1 for non-existent thread" << std::endl; return; }
    // negative tid
    if (uthread_terminate(-1) != -1) { std::cout << "FAIL: should return -1 for negative tid" << std::endl; return; }
    // tid out of range
    if (uthread_terminate(MAX_THREAD_NUM) != -1) { std::cout << "FAIL: should return -1 for tid >= MAX_THREAD_NUM" << std::endl; return; }
    std::cout << "PASS" << std::endl;
}

void test_spawn_null_entry() {
    std::cout << "\n=== Test: Spawn with NULL entry point ===" << std::endl;
    if (uthread_spawn(nullptr) != -1) { std::cout << "FAIL: should return -1 for null entry point" << std::endl; return; }
    std::cout << "PASS" << std::endl;
}

void test_id_reuse_order() {
    std::cout << "\n=== Test: ID Reuse Order (min ID first) ===" << std::endl;
    // spawn 5 threads (ids 1-5, assuming fresh state after previous terminates)
    int ids[5];
    for (int i = 0; i < 5; i++) {
        ids[i] = uthread_spawn(do_nothing);
        if (ids[i] == -1) { std::cout << "FAIL: spawn failed at i=" << i << std::endl; return; }
    }
    // terminate 3, 1, 5 — next spawns should get 1, 3, 5 in that order
    uthread_terminate(ids[2]); // free id 3
    uthread_terminate(ids[0]); // free id 1
    uthread_terminate(ids[4]); // free id 5

    int a = uthread_spawn(do_nothing); // should get 1
    int b = uthread_spawn(do_nothing); // should get 3
    int c = uthread_spawn(do_nothing); // should get 5

    if (a != ids[0] || b != ids[2] || c != ids[4]) {
        std::cout << "FAIL: expected IDs " << ids[0] << "," << ids[2] << "," << ids[4]
                  << " got " << a << "," << b << "," << c << std::endl;
    } else {
        std::cout << "PASS" << std::endl;
    }
    // cleanup
    uthread_terminate(a);
    uthread_terminate(b);
    uthread_terminate(c);
    for (int i = 1; i < 4; i++) uthread_terminate(ids[i]);
}

void test_spawn_terminate_repeat() {
    std::cout << "\n=== Test: Repeated Spawn/Terminate (memory leak check) ===" << std::endl;
    // do this 1000 times — valgrind will catch leaks
    for (int round = 0; round < 1000; round++) {
        int tid = uthread_spawn(do_nothing);
        if (tid == -1) { std::cout << "FAIL: spawn failed at round " << round << std::endl; return; }
        if (uthread_terminate(tid) != 0) { std::cout << "FAIL: terminate failed at round " << round << std::endl; return; }
    }
    std::cout << "PASS (run with valgrind to confirm no leaks)" << std::endl;
}

void test_double_terminate() {
    std::cout << "\n=== Test: Double Terminate ===" << std::endl;
    int tid = uthread_spawn(do_nothing);
    if (tid == -1) { std::cout << "FAIL: spawn failed" << std::endl; return; }
    if (uthread_terminate(tid) != 0) { std::cout << "FAIL: first terminate failed" << std::endl; return; }
    // terminating already-dead thread should return -1
    if (uthread_terminate(tid) != -1) { std::cout << "FAIL: second terminate should return -1" << std::endl; return; }
    std::cout << "PASS" << std::endl;
}

void test_fill_and_empty() {
    std::cout << "\n=== Test: Fill All Slots Then Empty ===" << std::endl;
    int ids[MAX_THREAD_NUM - 1];
    // fill all 99 slots
    for (int i = 0; i < MAX_THREAD_NUM - 1; i++) {
        ids[i] = uthread_spawn(do_nothing);
        if (ids[i] == -1) { std::cout << "FAIL: spawn failed at i=" << i << std::endl; return; }
    }
    // one more should fail
    if (uthread_spawn(do_nothing) != -1) { std::cout << "FAIL: should be full" << std::endl; return; }
    // terminate all
    for (int i = 0; i < MAX_THREAD_NUM - 1; i++) {
        if (uthread_terminate(ids[i]) != 0) { std::cout << "FAIL: terminate failed at i=" << i << std::endl; return; }
    }
    // now we should be able to spawn again
    int tid = uthread_spawn(do_nothing);
    if (tid != 1) { std::cout << "FAIL: expected tid=1 after full clear, got " << tid << std::endl; return; }
    uthread_terminate(tid);
    std::cout << "PASS" << std::endl;
}

int main() {
    std::cout << "Initializing thread library..." << std::endl;
    test_invalid_init();

    if (uthread_init(1000) != 0) {
        std::cout << "FAIL: uthread_init failed" << std::endl;
        return 1;
    }

    test_terminate_nonexistent();
    test_spawn_null_entry();
    test_id_reuse_order();
    test_spawn_terminate_repeat();
    test_double_terminate();
    test_fill_and_empty();

    std::cout << "\n=== All tests done ===" << std::endl;
    return 0;
}