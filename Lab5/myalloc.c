#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "myalloc.h"

void *_arena_start = NULL;  // Start of the arena
void *_arena_end = NULL;    // End of the arena
size_t _arena_size = 0;     // Size of the arena
int statusno = 0;

node_t *free_list = NULL;   // Head of the free list

int myinit(size_t size) {
    size_t page_size = sysconf(_SC_PAGESIZE);

    // Check if size exceeds MAX_ARENA_SIZE or is zero/negative
    if (size <= 0 || size > MAX_ARENA_SIZE) {
        statusno = ERR_BAD_ARGUMENTS;
        return ERR_BAD_ARGUMENTS;
    }

    // Adjust size to be a multiple of page size
    size = (size + page_size - 1) & ~(page_size - 1);

    printf("Initializing arena:\n");
    printf("...requested size %zu bytes\n", size);
    printf("...pagesize is %zu bytes\n", page_size);
    printf("...adjusting size with page boundaries\n");
    printf("...adjusted size is %zu bytes\n", size);

    // Allocate memory using mmap
    _arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (_arena_start == MAP_FAILED) {
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    _arena_size = size;
    _arena_end = _arena_start + size;

    // Initialize free list with a single large free node
    free_list = (node_t*)_arena_start;
    free_list->size = size - sizeof(node_t);
    free_list->is_free = 1;
    free_list->fwd = NULL;
    free_list->bwd = NULL;

    printf("...mapping arena with mmap()\n");
    printf("...arena starts at %p\n", _arena_start);
    printf("...arena ends at %p\n", _arena_end);
    printf("...initializing header for initial free chunk\n");
    printf("...header size is %zu bytes\n", sizeof(node_t));

    return _arena_size;
}

int mydestroy() {
    if (_arena_start == NULL) {
        statusno = ERR_UNINITIALIZED;
        return ERR_UNINITIALIZED;
    }

    printf("Destroying Arena:\n");

    // Unmap the arena memory
    if (munmap(_arena_start, _arena_size) == -1) {
        statusno = ERR_SYSCALL_FAILED;
        return ERR_SYSCALL_FAILED;
    }

    printf("...unmapping arena with munmap()\n");

    // Reset arena state
    _arena_start = NULL;
    _arena_end = NULL;
    _arena_size = 0;
    free_list = NULL;

    statusno = 0;
    return 0;
}

void* myalloc(size_t size) {
    if (size <= 0 || _arena_start == NULL) {
        statusno = ERR_BAD_ARGUMENTS;
        return NULL;
    }

    node_t *current = free_list;

    // Find a free node that fits the requested size
    while (current != NULL && (!(current->is_free) || current->size < size)) {
        current = current->fwd;
    }

    if (current == NULL) {
        statusno = ERR_OUT_OF_MEMORY;
        return NULL;
    }

    // Split the node if there's excess space
    if (current->size >= size + sizeof(node_t) + 1) {
        node_t *new_node = (node_t*)((char*)current + sizeof(node_t) + size);
        new_node->size = current->size - size - sizeof(node_t);
        new_node->is_free = 1;
        new_node->fwd = current->fwd;
        new_node->bwd = current;
        
        if (current->fwd) current->fwd->bwd = new_node;
        current->fwd = new_node;
        current->size = size;
    }

    // Mark current node as allocated
    current->is_free = 0;
    statusno = 0;
    return (void*)((char*)current + sizeof(node_t));
}

void myfree(void *ptr) {
    if (ptr == NULL || ptr < _arena_start || ptr >= _arena_end) {
        statusno = ERR_BAD_ARGUMENTS;
        return;
    }

    node_t *node = (node_t*)((char*)ptr - sizeof(node_t));
    node->is_free = 1;

    // Coalesce with forward node if it's free
    if (node->fwd && node->fwd->is_free) {
        node->size += sizeof(node_t) + node->fwd->size;
        node->fwd = node->fwd->fwd;
        if (node->fwd) node->fwd->bwd = node;
    }

    // Coalesce with backward node if it's free
    if (node->bwd && node->bwd->is_free) {
        node->bwd->size += sizeof(node_t) + node->size;
        node->bwd->fwd = node->fwd;
        if (node->fwd) node->fwd->bwd = node->bwd;
    }

    statusno = 0;
}
