#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"
#include "MemoryConstants.h"
#include <cmath>
#include <algorithm>

using namespace std;
// ============================================================================
// Helper Functions - Bit Manipulations & Math
// ============================================================================

/**
 * Extracts the offset (d) from a given virtual address.
 * @param virtualAddress The full virtual address.
 * @return The offset (the rightmost bits).
 */
uint64_t get_offset(uint64_t virtualAddress) {
    uint64_t mask = (1ULL << OFFSET_WIDTH) - 1;
    return virtualAddress & mask;
}

/**
 * Extracts the table index (p_i) for a specific depth in the tree.
 * @param virtualAddress The full virtual address.
 * @param depth The current depth in the tree (e.g., 0 for root, TABLES_DEPTH-1 for the lowest table).
 * @return The relevant index for this layer.
 */
uint64_t get_index(uint64_t virtualAddress, int depth) {
    // TODO: Implement using bitwise SHIFT (>>) and AND (&) operations
    int numToShift = OFFSET_WIDTH*(TABLES_DEPTH - depth);
    uint64_t newVA = virtualAddress >> numToShift;
    int mask = (1ULL << OFFSET_WIDTH) - 1; // creating mask for the OFFSET_WIDTH most right bits
    return newVA & mask;
}

/**
 * Calculates the minimal cyclic distance between two virtual pages.
 * @param page_swapped_in The virtual page currently being swapped in.
 * @param candidate_page The virtual page being considered for eviction.
 * @return The cyclic distance.
 */
uint64_t get_cyclic_distance(uint64_t page_swapped_in, uint64_t candidate_page) {
    if (page_swapped_in > candidate_page) {
        return min(page_swapped_in-candidate_page, uint64_t(NUM_PAGES)-page_swapped_in + candidate_page);
    }
    return min(candidate_page - page_swapped_in, uint64_t(NUM_PAGES)-candidate_page + page_swapped_in);

}

// ============================================================================
// Helper Functions - Memory Management & Tree Traversal (DFS)
// ============================================================================

/**
 * Depth-First Search (DFS) on the physical memory to gather frame data.
 * Updates the output parameters passed by reference.
 *
 * @param current_frame The physical frame currently being scanned.
 * @param depth The current depth in the tree.
 * @param current_page The virtual page number represented by the current path.
 * @param page_swapped_in The new page we want to bring in (for distance calculation).
 * @param frame_to_protect A frame that must not be evicted/unlinked (e.g., a newly created table).
 * @param parent_frame The frame index of the parent table.
 * @param parent_index The index within the parent table that points to current_frame.
 * * -- Out Parameters (updated during traversal) --
 * @param out_max_frame_index The highest frame index currently in use.
 * @param out_empty_table_frame The frame number of an empty table (if found).
 * @param out_empty_parent_frame The parent frame of the empty table.
 * @param out_empty_parent_index The index in the parent pointing to the empty table.
 * @param out_max_dist_page The page number with the maximal cyclic distance.
 * @param out_evict_frame The physical frame of the page to be evicted.
 * @param out_evict_parent_frame The parent frame of the evicted page.
 * @param out_evict_parent_index The index in the parent pointing to the evicted page.
 * @param out_max_dist The maximum cyclic distance found so far.
 */
void dfs(uint64_t current_frame, int depth, uint64_t current_page, uint64_t page_swapped_in, uint64_t frame_to_protect,
         uint64_t parent_frame, uint64_t parent_index,
         int& out_max_frame_index, uint64_t& out_empty_table_frame, uint64_t& out_empty_parent_frame, int& out_empty_parent_index,
         uint64_t& out_max_dist_page, uint64_t& out_evict_frame, uint64_t& out_evict_parent_frame, int& out_evict_parent_index, uint64_t& out_max_dist) {

    // TODO: Update out_max_frame_index if current_frame is larger
    // TODO: Check if leaf (depth == TABLES_DEPTH) -> It's a page! Calculate distance and update eviction candidates.
    // TODO: If not a leaf (it's a table) -> Check if it's completely empty (and not frame_to_protect). Update empty table candidates.
    // TODO: If not empty and not a leaf -> Iterate over its PAGE_SIZE entries. If entry != 0, call DFS recursively.
}

/**
 * Handles the Page Fault process to find and allocate a new physical frame.
 * Executes the DFS and selects a frame based on the 3 priorities.
 * * @param page_swapped_in The virtual page that triggered the page fault.
 * @param frame_to_protect A frame in the current path that must be protected from eviction (0 if none).
 * @return The physical frame index allocated for the new page/table.
 */
uint64_t find_frame(uint64_t page_swapped_in, uint64_t frame_to_protect) {
    int max_frame_index = 0;
    uint64_t empty_table_frame = 0, empty_parent_frame = 0;
    int empty_parent_index = 0;
    uint64_t max_dist_page = 0, evict_frame = 0, evict_parent_frame = 0, max_dist = 0;
    int evict_parent_index = 0;

    dfs(0, 0, 0, page_swapped_in, frame_to_protect, 0, 0,
        max_frame_index, empty_table_frame, empty_parent_frame, empty_parent_index,
        max_dist_page, evict_frame, evict_parent_frame, evict_parent_index, max_dist);

    if (empty_table_frame != 0) {
        PMwrite(empty_parent_frame * PAGE_SIZE + empty_parent_index, 0);
        return empty_table_frame;
    }

    if (max_frame_index + 1 < NUM_FRAMES) {
        return max_frame_index + 1;
    }

    PMevict(evict_frame, max_dist_page);
    PMwrite(evict_parent_frame * PAGE_SIZE + evict_parent_index, 0);
    return evict_frame;
}

// ============================================================================
// Core API Functions (from VirtualMemory.h)
// ============================================================================

void VMinitialize() {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(i, 0);
    }
}

int VMread(uint64_t virtualAddress, word_t* value) {
    uint64_t current_frame = 0;
    uint64_t virtualPage = virtualAddress >> OFFSET_WIDTH;

    for (int depth = 0; depth < TABLES_DEPTH; depth++) {
        uint64_t index = get_index(virtualAddress, depth);
        uint64_t entry_address = current_frame * PAGE_SIZE + index;

        word_t next_frame;
        PMread(entry_address, &next_frame);

        if (next_frame == 0) {
            uint64_t new_frame = find_frame(virtualPage, current_frame);

            if (depth < TABLES_DEPTH - 1) {
                for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
                    PMwrite(new_frame * PAGE_SIZE + i, 0);
                }
            } else {
                PMrestore(new_frame, virtualPage);
            }

            PMwrite(entry_address, new_frame);
            next_frame = new_frame;
        }

        current_frame = next_frame;
    }

    uint64_t offset = get_offset(virtualAddress);
    PMread(current_frame * PAGE_SIZE + offset, value);

    return 1;
}
int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t current_frame = 0;
    uint64_t virtualPage = virtualAddress >> OFFSET_WIDTH;

    for (int depth = 0; depth < TABLES_DEPTH; ++depth) {
        uint64_t index = get_index(virtualAddress, depth);
        uint64_t entry_address = current_frame * PAGE_SIZE + index;

        word_t next_frame;
        PMread(entry_address, &next_frame);

        if (next_frame == 0) {
            uint64_t new_frame = find_frame(virtualPage, current_frame);

            if (depth < TABLES_DEPTH - 1) {
                for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
                    PMwrite(new_frame * PAGE_SIZE + i, 0);
                }
            } else {
                PMrestore(new_frame, virtualPage);
            }

            PMwrite(entry_address, new_frame);
            next_frame = new_frame;
        }

        current_frame = next_frame;
    }

    uint64_t offset = get_offset(virtualAddress);
    PMwrite(current_frame * PAGE_SIZE + offset, value); // the only difference from read

    return 1;
}

uint64_t VMgetMapping(uint64_t virtualPage) {

    uint64_t current_frame = 0;
    for (int depth = 0; depth < TABLES_DEPTH; ++depth) {
        uint64_t index = get_index(virtualPage, depth);
        uint64_t entry_address = current_frame * PAGE_SIZE + index;
    }
    // TODO: Safely traverse the tree (read-only, no allocations or find_frame calls).
    // TODO: If a 0 is encountered at any level, return 0 immediately.
    // TODO: If the physical page is reached, return its frame index.
    return 0;
}