#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include "MemoryConstants.h"
#include <cmath>
#include <algorithm>

// for useing "min" instead of "std::min"
using namespace std;

uint64_t get_offset(uint64_t virtualAddress) {
    uint64_t mask = (1ULL << OFFSET_WIDTH) - 1;
    return virtualAddress & mask;
}


uint64_t get_index(uint64_t virtualAddress, int depth) {
    int numToShift = OFFSET_WIDTH*(TABLES_DEPTH - depth);
    uint64_t newVA = virtualAddress >> numToShift;
    int mask = (1ULL << OFFSET_WIDTH) - 1; // creating mask for the OFFSET_WIDTH most right bits
    return newVA & mask;
}


uint64_t get_cyclic_distance(uint64_t page_swapped_in, uint64_t candidate_page) {
    if (page_swapped_in > candidate_page) {
        return min(page_swapped_in-candidate_page, uint64_t(NUM_PAGES)-page_swapped_in + candidate_page);
    }
    return min(candidate_page - page_swapped_in, uint64_t(NUM_PAGES)-candidate_page + page_swapped_in);

}


void dfs(uint64_t current_frame, int depth, uint64_t current_page, uint64_t page_swapped_in, uint64_t frame_to_protect,
         uint64_t parent_frame, uint64_t parent_index,
         int& out_max_frame_index, uint64_t& out_empty_table_frame, uint64_t& out_empty_parent_frame, int& out_empty_parent_index,
         uint64_t& out_max_dist_page, uint64_t& out_evict_frame, uint64_t& out_evict_parent_frame, int& out_evict_parent_index, uint64_t& out_max_dist) {

    if((int)current_frame > out_max_frame_index)
        out_max_frame_index = current_frame;


    if (depth == TABLES_DEPTH) {
        int dis = get_cyclic_distance(current_page, page_swapped_in);
        if ((dis == out_max_dist && (current_page < out_max_dist_page)) || dis > out_max_dist) {
            // we found a candidate with better cyclic distance - replace it with current
            out_max_dist = dis;
            out_max_dist_page = current_page;
            out_evict_frame = current_frame;
            out_evict_parent_frame = parent_frame;
            out_evict_parent_index = (int)parent_index;
        }
        return;
    }
    bool is_empty = true;

    for (uint64_t i = 0; i < PAGE_SIZE; i++) {
        word_t next_frame;
        // read the content of the i row
        PMread(current_frame * PAGE_SIZE + i, &next_frame);
        if (next_frame != 0) {
            is_empty = false;

            uint64_t next_page = (current_page << OFFSET_WIDTH) | i;

            dfs(next_frame, depth + 1, next_page, page_swapped_in, frame_to_protect,
                    current_frame, i,
                    out_max_frame_index, out_empty_table_frame, out_empty_parent_frame, out_empty_parent_index,
                    out_max_dist_page, out_evict_frame, out_evict_parent_frame, out_evict_parent_index, out_max_dist);
        }
        }
    if (is_empty && current_frame != 0 && current_frame != frame_to_protect) {
        if (out_empty_table_frame == 0) {
            out_empty_table_frame = current_frame;
            out_empty_parent_frame = parent_frame;
            out_empty_parent_index = parent_index;
        }
    }
}


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

void VMinitialize() {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(i, 0);
    }
}

int VMread(uint64_t virtualAddress, word_t* value) {

    if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
        return 0;
    }

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

    if (virtualAddress >= VIRTUAL_MEMORY_SIZE) {
        return 0;
    }

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

// same logic but simplier
uint64_t VMgetMapping(uint64_t virtualPage) {
    uint64_t current_frame = 0;

    uint64_t virtualAddress = virtualPage << OFFSET_WIDTH;

    for (int depth = 0; depth < TABLES_DEPTH; ++depth) {
        uint64_t index = get_index(virtualAddress, depth);
        uint64_t entry_address = current_frame * PAGE_SIZE + index;
        word_t next_frame;
        PMread(entry_address, &next_frame);

        if (next_frame == 0) {
            return 0;
        }
        current_frame = next_frame;
    }

    return current_frame;
}