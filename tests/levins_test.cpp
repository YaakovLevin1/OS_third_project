#include "VirtualMemory.h"
#include <iostream>
#include <cassert>
#include <vector>

// Helper to print test status
void print_test_status(const std::string& test_name, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << test_name << std::endl;
    } else {
        std::cout << "[FAIL] " << test_name << " !!!" << std::endl;
    }
}

// ---------------------------------------------------------
// Test 1: Basic Boundaries and Offsets
// ---------------------------------------------------------
bool test_basic_and_offsets() {
    VMinitialize();
    word_t value;

    // Test extreme boundaries
    VMwrite(0, 111);
    VMwrite(VIRTUAL_MEMORY_SIZE - 1, 999);

    VMread(0, &value);
    if (value != 111) return false;

    VMread(VIRTUAL_MEMORY_SIZE - 1, &value);
    if (value != 999) return false;

    // Test different offsets within the same page (Page 1)
    for (uint64_t offset = 0; offset < PAGE_SIZE; ++offset) {
        VMwrite(PAGE_SIZE + offset, offset + 100);
    }

    for (uint64_t offset = 0; offset < PAGE_SIZE; ++offset) {
        VMread(PAGE_SIZE + offset, &value);
        if (value != (word_t)(offset + 100)) return false;
    }

    return true;
}

// ---------------------------------------------------------
// Test 2: VMgetMapping Safety and Correctness
// ---------------------------------------------------------
bool test_get_mapping_safety() {
    VMinitialize();
    
    // Attempt to get mapping for an unmapped page (should be 0)
    uint64_t unmapped_page = 5;
    uint64_t frame = VMgetMapping(unmapped_page);
    if (frame != 0) return false;

    // Write to that page to map it
    VMwrite(unmapped_page * PAGE_SIZE, 42);

    // Now it should be mapped to a valid frame (>0)
    frame = VMgetMapping(unmapped_page);
    if (frame == 0) return false;

    // Verify it didn't map neighboring pages by accident
    if (VMgetMapping(unmapped_page + 1) != 0) return false;
    if (VMgetMapping(unmapped_page - 1) != 0) return false;

    return true;
}

// ---------------------------------------------------------
// Test 3: Heavy Eviction and Restore
// ---------------------------------------------------------
bool test_heavy_eviction_restore() {
    VMinitialize();
    word_t value;

    // Write to the first word of EVERY virtual page.
    // Since NUM_PAGES > NUM_FRAMES, this forces massive evictions.
    for (uint64_t i = 0; i < NUM_PAGES; ++i) {
        VMwrite(i * PAGE_SIZE, (word_t)i * 10);
    }

    // Now read them back. This forces massive restores from the swap file.
    for (uint64_t i = 0; i < NUM_PAGES; ++i) {
        VMread(i * PAGE_SIZE, &value);
        if (value != (word_t)(i * 10)) {
            std::cout << "  -> Mismatch at page " << i << ". Expected: " << i * 10 << " Got: " << value << std::endl;
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------
// Test 4: Extreme Stress Test
// ---------------------------------------------------------
bool test_stress() {
    VMinitialize();
    word_t value;
    
    // A deterministic pseudo-random sequence of pages
    std::vector<uint64_t> access_order;
    for(uint64_t i = 0; i < NUM_PAGES; ++i) {
        // Jump around to confuse the eviction algorithm and tree traversal
        access_order.push_back((i * 17 + 3) % NUM_PAGES); 
    }

    // Write phase
    for (uint64_t page : access_order) {
        uint64_t addr = page * PAGE_SIZE + (page % PAGE_SIZE); // varied offset
        VMwrite(addr, (word_t)page * 7);
    }

    // Read phase
    for (uint64_t page : access_order) {
        uint64_t addr = page * PAGE_SIZE + (page % PAGE_SIZE);
        VMread(addr, &value);
        if (value != (word_t)(page * 7)) return false;
    }

    return true;
}

// ---------------------------------------------------------
// Main
// ---------------------------------------------------------
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  Starting Comprehensive Virtual Memory Test" << std::endl;
    std::cout << "  VIRTUAL_MEMORY_SIZE: " << VIRTUAL_MEMORY_SIZE << std::endl;
    std::cout << "  RAM_SIZE: " << RAM_SIZE << std::endl;
    std::cout << "  NUM_PAGES: " << NUM_PAGES << std::endl;
    std::cout << "  NUM_FRAMES: " << NUM_FRAMES << std::endl;
    std::cout << "========================================" << std::endl;

    bool t1 = test_basic_and_offsets();
    print_test_status("Test 1: Basic & Offsets", t1);

    bool t2 = test_get_mapping_safety();
    print_test_status("Test 2: VMgetMapping Safety", t2);

    bool t3 = test_heavy_eviction_restore();
    print_test_status("Test 3: Heavy Eviction & Restore", t3);

    bool t4 = test_stress();
    print_test_status("Test 4: Extreme Stress Test", t4);

    std::cout << "========================================" << std::endl;
    if (t1 && t2 && t3 && t4) {
        std::cout << "ALL TESTS PASSED SUCCESSFULLY! YOU ARE READY!" << std::endl;
    } else {
        std::cout << "SOME TESTS FAILED. CHECK THE LOGIC." << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return 0;
}