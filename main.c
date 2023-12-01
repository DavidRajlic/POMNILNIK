// MAP_ANONYMOUS
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "mymalloc.h"

void
fill_buffer(void *buffer, size_t size)
{
    // Fill with numbers from 0 to 255
    for (size_t i = 0; i < size; i++) {
        ((unsigned char *)buffer)[i] = i % 256;
    }
}

bool
check_buffer(void *buffer, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (((unsigned char *)buffer)[i] != i % 256) {
            return false;
        }
    }

    return true;
}

//==============================================================================
// Test 1
//==============================================================================

bool
test_single(size_t size)
{
    void *ptr = mymalloc(size);
    fill_buffer(ptr, size);
    bool ret = check_buffer(ptr, size);
    myfree(ptr);
    
    return ret;
}


//==============================================================================
// Test 2
//==============================================================================

bool
test_multiple_sequential(size_t size, int times)
{
    bool ret = true;

    for (int i=0; i<times; i++) {
        void *ptr = mymalloc(size);
        fill_buffer(ptr, size);
        ret = ret && check_buffer(ptr, size);
        myfree(ptr);
    }

    return ret;
}

//==============================================================================
// Test 3
//==============================================================================

bool
test_multiple_parallel(size_t size, int times)
{
    bool ret = true;
    void *ptrs[times];

    for (int i=0; i<times; i++) {
        ptrs[i] = mymalloc(size);
        fill_buffer(ptrs[i], size);
    }

    for (int i=0; i<times; i++) {
        ret = ret && check_buffer(ptrs[i], size);
        myfree(ptrs[i]);
    }

    return ret;
}

//==============================================================================
// Test 4
//==============================================================================

bool
test_multiple_parallel_reversed(size_t size, int times)
{
    bool ret = true;
    void *ptrs[times];

    for (int i=0; i<times; i++) {
        ptrs[i] = mymalloc(size);
        fill_buffer(ptrs[i], size);
    }

    for (int i=times - 1; i>=0; i--) {
        ret = ret && check_buffer(ptrs[i], size);
        myfree(ptrs[i]);
    }

    return ret;
}

//==============================================================================
// Test 5
//==============================================================================

bool
test_multiple_parallel_random(size_t size)
{
    bool ret = true;
    void *ptrs[3];

    for (int i=0; i<3; i++) {
        ptrs[i] = mymalloc(size);
        fill_buffer(ptrs[i], size);
    }

    ret = ret && check_buffer(ptrs[1], size);
    myfree(ptrs[1]);

    ret = ret && check_buffer(ptrs[2], size);
    myfree(ptrs[2]);

    ret = ret && check_buffer(ptrs[0], size);
    myfree(ptrs[0]);

    return ret;
}

//==============================================================================
// Test 6
//==============================================================================

bool
test_multiple_3x_sequential()
{
    bool ret = true;
    
    ret = ret && test_multiple_sequential(10, 3);
    ret = ret && test_multiple_sequential(1024, 3);
    ret = ret && test_multiple_sequential(4 * 1024, 3);
    ret = ret && test_multiple_sequential(1024 * 1024, 3);

    return ret;
}

bool
test_multiple_3x_parallel()
{
    bool ret = true;

    ret = ret && test_multiple_parallel(10, 3);
    ret = ret && test_multiple_parallel(1024, 3);
    ret = ret && test_multiple_parallel(4 * 1024, 3);
    ret = ret && test_multiple_parallel(1024 * 1024, 3);

    return ret;
}

bool
test_multiple_3x_parallel_reversed()
{
    bool ret = true;

    ret = ret && test_multiple_parallel_reversed(10, 3);
    ret = ret && test_multiple_parallel_reversed(1024, 3);
    ret = ret && test_multiple_parallel_reversed(4 * 1024, 3);
    ret = ret && test_multiple_parallel_reversed(1024 * 1024, 3);

    return ret;
}

bool
test_multiple_3x_parallel_random()
{
    bool ret = true;

    ret = ret && test_multiple_parallel_random(10);
    ret = ret && test_multiple_parallel_random(1024);
    ret = ret && test_multiple_parallel_random(4 * 1024);
    ret = ret && test_multiple_parallel_random(1024 * 1024);

    return ret;
}

//==============================================================================
// Test 7
// Testing for error handling.
//==============================================================================

bool
test_error(size_t size)
{
    void *ptr = mymalloc(size);
    if (ptr == NULL) {
        return true;
    }

    return false;
}

bool
test_error_free()
{
    myfree(NULL);

    return true;
}

//==============================================================================
// Test 8
// Test if small allocations are alocated withing the same page.
//==============================================================================

bool
test_use_remaining_page()
{
    bool ret = true;

    int page_size = getpagesize();
    void *ptr_1 = mymalloc(128);
    void *ptr_2 = mymalloc(128);
    void *ptr_3 = mymalloc(128);
    
    long addr_1 = (long)ptr_1;
    long addr_2 = (long)ptr_2;
    long addr_3 = (long)ptr_3;
    
    // the distance between addresses must be smaller than page size
    if(abs(addr_2-addr_1) > page_size) {
        ret = false;        
    }
    if(abs(addr_3-addr_1) > page_size) {
        ret = false;        
    }
    if(abs(addr_3-addr_2) > page_size) {
        ret = false;        
    }
    
    myfree(ptr_1);
    myfree(ptr_2);
    myfree(ptr_3);

    return ret;
}

//==============================================================================
// Test 9
// Test if free'd memory segments can be reused later.
//==============================================================================

bool
test_reuse_segments()
{
    bool ret = true;

    void *ptr_1 = mymalloc(128);
    void *ptr_2 = mymalloc(128);
    
    myfree(ptr_1);
    void *ptr_3 = mymalloc(128);
    
    // we assume that after prt_1 is freed it can be reallocated
    ret = ptr_3==ptr_1;
    
    myfree(ptr_2);
    myfree(ptr_3);

    return ret;
}

//==============================================================================
// Test 10 
// Test if small freed segments are joined together into larger segments that
// can used for larger allocations.
//==============================================================================

bool
test_join_segments()
{
    bool ret = true;

    void *ptr_1 = mymalloc(128);
    void *ptr_2 = mymalloc(128);
    void *ptr_3 = mymalloc(128);
    
    myfree(ptr_1);
    myfree(ptr_2);
    
    void *ptr_4 = mymalloc(256);
    
    // after freeing segments in ptr_1 and ptr_2 
    // we should have enough space for 256 bytes in ptr_4
    // we assume the same address as ptr_1 is returned
    ret = ptr_1==ptr_4;
    
    myfree(ptr_3);
    myfree(ptr_4);

    return ret;
}

//==============================================================================
// Test 11 
// Test if pages with no allocated segments are freed.
//==============================================================================

bool
test_free_empty_pages()
{
    int page_size = getpagesize();
    bool ret = true;

    void *ptr_1 = mymalloc(128); 
    void *ptr_2 = mymalloc(128); 
    myfree(ptr_1);
    myfree(ptr_2); // should free the first page

    void *ptr_3 = mymalloc(page_size-page_size*3/4); // close to pagesize
    myfree(ptr_3); // should free the second page

    void *ptr_4 = mymalloc(page_size*3/2); // larger than one page size
    myfree(ptr_4); // should free the third and fourth page

    return ret;
}

//==============================================================================
// Main
//==============================================================================

int
main(int argc, char **argv)
{
    if (argc < 2) {
        printf("ERROR: Specify test name.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "run_mem_alloc_prof") == 0 && argc < 3) {
        printf("ERROR: Specify path to memory allocation profile.\n");
        return EXIT_FAILURE;
    }



    // BEGIN_STRACE
    mmap(NULL, 0, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    bool test_res = false;

    if      (strcmp(argv[1], "test_10B") == 0) test_res = test_single(10    );
    else if (strcmp(argv[1], "test_1kB") == 0) test_res = test_single(1024);
    else if (strcmp(argv[1], "test_4kB") == 0) test_res = test_single(4 * 1024);
    else if (strcmp(argv[1], "test_1MB") == 0) test_res = test_single(1024 * 1024);

    else if (strcmp(argv[1], "test_3x10B_sequential") == 0) test_res = test_multiple_sequential(10, 3);
    else if (strcmp(argv[1], "test_3x1kB_sequential") == 0) test_res = test_multiple_sequential(1024, 3);
    else if (strcmp(argv[1], "test_3x4kB_sequential") == 0) test_res = test_multiple_sequential(4 * 1024, 3);
    else if (strcmp(argv[1], "test_3x1MB_sequential") == 0) test_res = test_multiple_sequential(1024 * 1024, 3);
    
    else if (strcmp(argv[1], "test_3x10B_parallel") == 0) test_res = test_multiple_parallel(10, 3);
    else if (strcmp(argv[1], "test_3x1kB_parallel") == 0) test_res = test_multiple_parallel(1024, 3);
    else if (strcmp(argv[1], "test_3x4kB_parallel") == 0) test_res = test_multiple_parallel(4 * 1024, 3);
    else if (strcmp(argv[1], "test_3x1MB_parallel") == 0) test_res = test_multiple_parallel(1024 * 1024, 3);
    
    else if (strcmp(argv[1], "test_3x10B_parallel_reversed") == 0) test_res = test_multiple_parallel_reversed(10, 3);
    else if (strcmp(argv[1], "test_3x1kB_parallel_reversed") == 0) test_res = test_multiple_parallel_reversed(1024, 3);
    else if (strcmp(argv[1], "test_3x4kB_parallel_reversed") == 0) test_res = test_multiple_parallel_reversed(4 * 1024, 3);
    else if (strcmp(argv[1], "test_3x1MB_parallel_reversed") == 0) test_res = test_multiple_parallel_reversed(1024 * 1024, 3);
    
    else if (strcmp(argv[1], "test_3x10B_parallel_random") == 0) test_res = test_multiple_parallel_random(10);
    else if (strcmp(argv[1], "test_3x1kB_parallel_random") == 0) test_res = test_multiple_parallel_random(1024);
    else if (strcmp(argv[1], "test_3x4kB_parallel_random") == 0) test_res = test_multiple_parallel_random(4 * 1024);
    else if (strcmp(argv[1], "test_3x1MB_parallel_random") == 0) test_res = test_multiple_parallel_random(1024 * 1024);

    else if (strcmp(argv[1], "test_multiple_3x_sequential") == 0) test_res = test_multiple_3x_sequential();
    else if (strcmp(argv[1], "test_multiple_3x_parallel") == 0) test_res = test_multiple_3x_parallel();
    else if (strcmp(argv[1], "test_multiple_3x_parallel_reversed") == 0) test_res = test_multiple_3x_parallel_reversed();
    else if (strcmp(argv[1], "test_multiple_3x_parallel_random") == 0) test_res = test_multiple_3x_parallel_random();

    else if (strcmp(argv[1], "test_error_0_B") == 0) test_res = test_error(0);
    else if (strcmp(argv[1], "test_error_1_TB") == 0) test_res = test_error((size_t)1024 * 1024 * 1024 * 1024);
    else if (strcmp(argv[1], "test_error_NULL") == 0) test_res = test_error_free();
    
    else if (strcmp(argv[1], "test_use_remaining_page") == 0) test_res = test_use_remaining_page();
    else if (strcmp(argv[1], "test_reuse_segment") == 0) test_res = test_reuse_segments();
    else if (strcmp(argv[1], "test_join_segments") == 0) test_res = test_join_segments();
    else if (strcmp(argv[1], "test_free_empty_page") == 0) test_res = test_free_empty_pages();

    else { printf("ERROR: Invalid test name.\n"); test_res = false; }
    printf("Test single zaključen: %s\n", test_res ? "true" : "false");
    

    return test_res ? EXIT_SUCCESS : EXIT_FAILURE;
}
