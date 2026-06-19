#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <iostream>
#include <cassert>

void test_protect_ancestors() {
    VMinitialize();

    // 1. ממלאים את כל הזיכרון הפיזי כדי להבטיח שאין מסגרות פנויות (RAM Full)
    // אנחנו כותבים לכתובות שמפוזרות מספיק כדי ליצור טבלאות ודפים רבים
    for (uint64_t i = 0; i < NUM_FRAMES * PAGE_SIZE * 2; i += PAGE_SIZE) {
        VMwrite(i, i);
    }

    // 2. עכשיו הזיכרון מלא. נכתוב לדף רחוק מאוד (כתובת חדשה לחלוטין).
    // פעולה זו תחייב לפחות 2-3 הקצאות של מסגרות חדשות (עבור הטבלאות ועבור הדף).
    // אם frame_to_protect לא ממומש נכון, המערכת תפנה את אחת הטבלאות של עצמה,
    // וכשתנסה לכתוב לתוכה היא תדרוס זיכרון לא נכון או תקרוס (Segfault).
    uint64_t far_address = (VIRTUAL_MEMORY_SIZE / 2) + PAGE_SIZE;
    VMwrite(far_address, 999);

    word_t val = 0;
    VMread(far_address, &val);
    assert(val == 999);

    std::cout << "Protect Ancestors test passed!" << std::endl;
}

void test_read_unmapped() {
    VMinitialize();
    word_t val = 42; // אתחול עם ערך זבל כדי לוודא ש-VMread דורס אותו

    // 1. קריאה מכתובת חוקית אך כזו שמעולם לא פנינו אליה
    uint64_t unmapped_address = PAGE_SIZE * 3;
    VMread(unmapped_address, &val);

    // מאחר והדף מעולם לא נכתב אליו, הדיסק יחזיר 0 בעת ה-Swap In
    assert(val == 0);

    // 2. נוודא שהקצאת העץ התבצעה כראוי על ידי כתיבה לאותה כתובת לאחר מכן
    VMwrite(unmapped_address, 84);
    VMread(unmapped_address, &val);
    assert(val == 84);

    std::cout << "Read Unmapped Page test passed!" << std::endl;
}

void test_cyclical_tie() {
    VMinitialize();

    // ניצור כתיבה ל-3 קצוות שונים של הזיכרון הווירטואלי
    // במטרה ליצור מרחקים זהים כשנכניס דף חדש באמצע
    uint64_t page_a = 1 * PAGE_SIZE;
    uint64_t page_b = (NUM_PAGES / 2) * PAGE_SIZE;
    uint64_t page_c = (NUM_PAGES - 1) * PAGE_SIZE;

    VMwrite(page_a, 111);
    VMwrite(page_b, 222);
    VMwrite(page_c, 333);

    // עכשיו נכריח פינויים על ידי כתיבה לעוד הרבה דפים
    // זה יגרום למערכת לחפש קורבנות ולחשב מרחקים מחזוריים
    for (uint64_t i = 2; i < 6; ++i) {
        if ((i * PAGE_SIZE) != page_b && (i * PAGE_SIZE) != page_c) {
            VMwrite(i * PAGE_SIZE, i * 10);
        }
    }

    // נוודא שהנתונים המקוריים שרדו או הוחזרו בהצלחה מהדיסק
    // תוך כדי שמירה על המבנה המחזורי
    word_t val_a, val_b, val_c;
    VMread(page_a, &val_a);
    VMread(page_b, &val_b);
    VMread(page_c, &val_c);

    assert(val_a == 111);
    assert(val_b == 222);
    assert(val_c == 333);

    std::cout << "Cyclical Tie-Breaker test passed!" << std::endl;
}

int main() {
    test_read_unmapped();
    test_protect_ancestors();
    test_cyclical_tie();

    return 0;
}
