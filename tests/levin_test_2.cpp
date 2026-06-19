#include "VirtualMemory.h"
#include <iostream>
#include <cassert>

// פונקציית עזר להדפסת סטטוס חזותי ברור
void print_status(const std::string& msg, bool success) {
    if (success) {
        std::cout << " [V] PASSED: " << msg << std::endl;
    } else {
        std::cerr << " [X] FAILED: " << msg << " !!!" << std::endl;
    }
}

// ---------------------------------------------------------
// Test 1: Adjacency & Table Reuse
// מוודא שכתיבה לדפים סמוכים משתמשת נכון בטבלאות קיימות
// ---------------------------------------------------------
bool test_adjacency_and_table_reuse() {
    VMinitialize();
    
    // כותבים ל-3 הדפים הראשונים ברצף
    for (uint64_t i = 0; i < PAGE_SIZE * 3; ++i) {
        VMwrite(i, i + 10);
    }
    
    word_t val;
    for (uint64_t i = 0; i < PAGE_SIZE * 3; ++i) {
        VMread(i, &val);
        if (val != (word_t)(i + 10)) {
            std::cerr << "Mismatch at address " << i << std::endl;
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------
// Test 2: Cyclic Tie-Breaker Resolution
// מאלץ שוויון במרחק המעגלי כדי לבחון את תנאי "האינדקס הנמוך מנצח"
// ---------------------------------------------------------
bool test_cyclic_tie_breaker() {
    VMinitialize();
    
    // כותבים למספר דפים המרוחקים זה מזה בצורה סימטרית
    uint64_t p1 = 1;
    uint64_t p2 = NUM_PAGES - 1;
    uint64_t p3 = NUM_PAGES / 2;
    
    // מונע קריסה אם הזיכרון קטן מאוד
    if (NUM_PAGES < 4) return true; 
    
    VMwrite(p1 * PAGE_SIZE, 100);
    VMwrite(p2 * PAGE_SIZE, 200);
    VMwrite(p3 * PAGE_SIZE, 300);
    
    // מציפים את הזיכרון הפיזי כדי לכפות פינוי ולגרות את אלגוריתם ה-DFS
    for(uint64_t i = 0; i < NUM_FRAMES + 2; ++i) {
        uint64_t safe_page = i % NUM_PAGES;
        VMwrite(safe_page * PAGE_SIZE, safe_page * 5);
    }
    
    // בדיקת שלמות המידע
    word_t val;
    for(uint64_t i = 0; i < NUM_FRAMES + 2; ++i) {
        uint64_t safe_page = i % NUM_PAGES;
        VMread(safe_page * PAGE_SIZE, &val);
        if (val != (word_t)(safe_page * 5)) return false;
    }
    return true;
}

// ---------------------------------------------------------
// Test 3: Sparse Branches & Empty Table Reclamation
// בודק את עדיפות 1 באלגוריתם: ניקוי טבלאות שהתרוקנו מדפים
// ---------------------------------------------------------
bool test_sparse_memory_and_empty_tables() {
    VMinitialize();
    word_t val;
    
    // יוצרים פיזור רחב מאוד של דפים כדי לפתוח ענפים רבים ושונים בעץ
    uint64_t step = NUM_PAGES / (NUM_FRAMES > 0 ? NUM_FRAMES : 2);
    if (step == 0) step = 1;
    
    for (uint64_t i = 0; i < NUM_PAGES; i += step) {
        VMwrite(i * PAGE_SIZE, i);
    }
    
    // כעת דורסים את הזיכרון הפיזי עם כתיבה צפופה ומסיבית למקטע אחד קטן.
    // הפעולה מאלצת פינוי של הדפים הרחוקים, מה שגורם לענפים שלמים להתרוקן.
    // ה-DFS חייב לזהות את הטבלאות הריקות הללו, לקצור אותן, ולהשתמש במסגרות מחדש.
    for (uint64_t i = 0; i < NUM_FRAMES * PAGE_SIZE; ++i) {
        // מגן מפני גלישה מעבר לגבולות הדפים המותרים
        if (i / PAGE_SIZE >= NUM_PAGES) break; 
        VMwrite(i, i * 2);
    }
    
    // מוודאים שהכתיבה הצפופה הצליחה וששום טבלה לא ניתקה במקום הלא נכון
    for (uint64_t i = 0; i < NUM_FRAMES * PAGE_SIZE; ++i) {
         if (i / PAGE_SIZE >= NUM_PAGES) break; 
        VMread(i, &val);
        if (val != (word_t)(i * 2)) return false;
    }
    return true;
}

// ---------------------------------------------------------
// Test 4: Deep Thrashing (Repeated Massive Swapping)
// ---------------------------------------------------------
bool test_thrashing() {
    VMinitialize();
    word_t val;
    
    // כתיבה וקריאה של כמות דפים כפולה מכמות המסגרות בזיכרון, 
    // לאורך 3 סבבים, כדי להבטיח אמינות שחזור (Restore) בתנאי קיצון.
    uint64_t pages_to_thrash = (NUM_FRAMES * 2 > NUM_PAGES) ? NUM_PAGES : NUM_FRAMES * 2;
    
    for (int round = 1; round <= 3; ++round) {
        // שלב כתיבה
        for (uint64_t i = 0; i < pages_to_thrash; ++i) {
            VMwrite(i * PAGE_SIZE, round * 1000 + i);
        }
        
        // שלב קריאה מידי באותו סבב
        for (uint64_t i = 0; i < pages_to_thrash; ++i) {
            VMread(i * PAGE_SIZE, &val);
            if (val != (word_t)(round * 1000 + i)) {
                std::cerr << "Thrashing failed at round " << round << ", page " << i << std::endl;
                return false;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main() {
    std::cout << "===============================================" << std::endl;
    std::cout << "  ADVANCED MECHANICS VIRTUAL MEMORY TEST       " << std::endl;
    std::cout << "===============================================" << std::endl;
    
    bool t1 = test_adjacency_and_table_reuse();
    print_status("Adjacency & Table Reuse", t1);

    bool t2 = test_cyclic_tie_breaker();
    print_status("Cyclic Tie-Breaker Resolution", t2);

    bool t3 = test_sparse_memory_and_empty_tables();
    print_status("Sparse Branches & Empty Table Reclamation", t3);

    bool t4 = test_thrashing();
    print_status("Deep Thrashing (Repeated Swapping)", t4);

    std::cout << "===============================================" << std::endl;
    if (t1 && t2 && t3 && t4) {
        std::cout << " ALL MECHANICS TESTS PASSED! EXCEPTIONAL WORK. " << std::endl;
    } else {
        std::cout << " SOME TESTS FAILED. CHECK DFS AND EVICTION LOGIC." << std::endl;
    }
    std::cout << "===============================================" << std::endl;

    return 0;
}