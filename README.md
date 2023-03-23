# Dynamic Memory Allocator (SFMM - System-Fundamentals-Memory-Manager)

## Overview

This project implements a dynamic memory allocator for a system that manages memory allocation and deallocation efficiently. The allocator supports various operations including `malloc`, `free`, `realloc`, and `memalign`, providing functionality similar to that of standard memory management libraries. The main objective is to ensure efficient memory usage while handling fragmentation and coalescence of memory blocks.

## Code Structure

The implementation for this assignment is organized as follows:

- **src/main.c:** This file is used to run methods directly from `src/sfmm.c` to check for functionality.
- **src/sfmm.c:** This file consists of functions implemented to fulfill the tasks given by the assignment. The core logic and functionality of the dynamic memory allocator are implemented here.
- **tests/sfmm_tests.c:** This file contains test cases, including the three test cases created by myself and additional test cases provided for evaluating the correctness and robustness of the implemented dynamic memory allocator.

The code structure helps maintain a clear separation between the main functionality, testing, and execution components of the assignment.

## Detailed Description

### Core Functions

#### sf_malloc(size_t size)

Allocates a memory block of at least `size` bytes. It ensures alignment and manages memory allocation from the free list, growing the heap if necessary.

#### sf_free(void *pp)

Frees a previously allocated memory block pointed to by `pp`. It coalesces adjacent free blocks to reduce fragmentation and inserts the freed block into the appropriate free list.

#### sf_realloc(void *pp, size_t rsize)

Reallocates a previously allocated memory block `pp` to a new size `rsize`. If the new size is smaller, the block may be split. If larger, a new block is allocated, and the old data is copied over.

#### sf_memalign(size_t size, size_t align)

Allocates a memory block of at least `size` bytes aligned to a multiple of `align`. It handles alignment constraints by splitting blocks appropriately and ensuring that the returned block satisfies the alignment requirement.

### Helper Functions

#### insert_flst(sf_block *ptr, sf_block *prev)

Inserts a block into the free list.

#### delete_flst(sf_block *ptr)

Removes a block from the free list.

#### insert_main_by_size(sf_block *ptr, size_t size)

Inserts a block into the appropriate free list based on its size.

#### check_flst_for_block(size_t alloc_size)

Searches the free list for a block of at least `alloc_size` bytes.

#### valid_pointer(void *pp)

Validates a pointer to ensure it is a valid allocated block within the heap bounds.

#### coalesce(void *ptr)

Coalesces adjacent free blocks to form larger blocks, reducing fragmentation and inserting the coalesced block back into the free list.

## Testing

### Test Cases

The `tests/sfmm_tests.c` file includes:

- **Custom Test Cases:** Three test cases created by myself to validate the core functionalities.
- **Provided Test Cases:** Additional test cases provided to evaluate the correctness and robustness of the implementation.

### Running Tests

To run the test cases, compile the tests with the main allocator implementation and execute the resulting binary. The tests ensure that the allocator handles various edge cases and typical usage scenarios effectively.

## Specification

For detailed specifications and requirements, please refer to the [SPEC.md](SPEC.md) file.

## Conclusion

This dynamic memory allocator project, known as SFMM (System-Fundamentals-Memory-Manager), demonstrates a comprehensive implementation of memory management functions. By organizing the code into clear modules and providing thorough testing, the project ensures a robust and efficient solution to dynamic memory allocation challenges.