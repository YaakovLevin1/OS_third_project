#include "../src/VirtualMemory.h"
#include <iostream>
#include <cassert>

void test_heavy_eviction() {
    VMinitialize();
    
    // כתיבה לכל דף וירטואלי במערכת כדי למלא את ה-RAM ולכפות פינויים
    for (uint64_t i = 0; i < VIRTUAL_MEMORY_SIZE; i += PAGE_SIZE) {
        VMwrite(i, i + 100);
    }

    // קריאה ווידוא שכל הערכים שרדו את הפינוי והשחזור מהדיסק
    for (uint64_t i = 0; i < VIRTUAL_MEMORY_SIZE; i += PAGE_SIZE) {
        word_t value;
        VMread(i, &value);
        assert(value == i + 100);
    }
    
    std::cout << "Heavy eviction test passed!" << std::endl;
}

void test_offsets() {
    VMinitialize();

    // כתיבה למילה הראשונה של דף מספר 1
    uint64_t start_of_page = PAGE_SIZE;
    // כתיבה למילה האחרונה של דף מספר 1
    uint64_t end_of_page = PAGE_SIZE + PAGE_SIZE - 1;

    VMwrite(start_of_page, 111);
    VMwrite(end_of_page, 222);

    word_t val1 = 0, val2 = 0;
    VMread(start_of_page, &val1);
    VMread(end_of_page, &val2);

    assert(val1 == 111);
    assert(val2 == 222);

    std::cout << "Offsets and boundaries test passed!" << std::endl;
}

void test_empty_tables() {
    VMinitialize();

    // כתיבה לדפים סמוכים (קרובים מאוד)
    VMwrite(0, 10);
    VMwrite(PAGE_SIZE, 20);

    // כתיבה לכתובות מפוזרות ורחוקות מאוד כדי לכפות פינוי של הדפים הראשונים
    // הפעולה הזו תרוקן את הטבלאות שהצביעו עליהם
    uint64_t jump = VIRTUAL_MEMORY_SIZE / 4;
    for (uint64_t i = jump; i < VIRTUAL_MEMORY_SIZE; i += jump) {
        VMwrite(i, i);
    }

    // קריאה חוזרת של הערכים המקוריים (יחייב יצירה מחדש של הטבלאות)
    word_t val1, val2;
    VMread(0, &val1);
    VMread(PAGE_SIZE, &val2);

    assert(val1 == 10);
    assert(val2 == 20);

    std::cout << "Empty tables reclaim test passed!" << std::endl;
}

int main() {
    test_heavy_eviction();
    test_offsets();
    test_empty_tables();
    return 0;
}