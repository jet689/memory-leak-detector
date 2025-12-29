#ifndef __LEAK_DETECTOR_H_
#define __LEAK_DETECTOR_H_

#ifdef __cpluscplus
extern "C"
{
#endif // __cpluscplus

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

//undefine if you wish to use with set size.
#define LEAK_MEM_DYNAMIC

#ifdef LEAK_MEM_DYNAMIC
#define LEAK_MEM_START_SIZE 500
#define LEAK_MEM_INCREMENT_SIZE 500
uint32_t LEAK_MEM_SIZE=LEAK_MEM_START_SIZE;
#else
#define LEAK_MEM_SIZE 1000
#endif

#define _leak_warn(file, line, msg) \
    printf("WARNING:: (%s:%d) %s\n", file, line, msg)

#undef malloc
#undef realloc
#undef free

static bool initialized = false;

typedef struct {
    size_t address;
    size_t size;
    char file[255];
    uint32_t line;
} Mem;

static struct MemData {
#ifdef LEAK_MEM_DYNAMIC
    Mem *mem;
#else
    Mem mem[LEAK_MEM_SIZE];
#endif

    uint32_t current;
    uint32_t allocations;
    uint32_t free;
    size_t total_allocated;
    size_t total_freed;
} memoryData;

void* mem_catch_alloc(void *p){
    if(p==NULL){
        printf("WARNING::Memory allocation for leak.h failed");
        exit(EXIT_FAILURE);
    }
    return p;
}

static bool _insert(void *ptr, size_t size, int line, char *file) {
    uint32_t i;
    const size_t address = (size_t)ptr;
    
#ifdef LEAK_MEM_DYNAMIC
    insert_to_mem:
#endif
    
    if ((i = memoryData.current) < LEAK_MEM_SIZE) {
        memoryData.mem[i].address = address;
        memoryData.mem[i].size = size;
        memoryData.mem[i].line = line;
        strcpy(memoryData.mem[i].file, file);

        memoryData.current++;
        memoryData.allocations++;
        memoryData.total_allocated += size;
        return true;
    }else{
#ifdef LEAK_MEM_DYNAMIC
        memoryData.mem=mem_catch_alloc(
                realloc(
                    memoryData.mem,sizeof(Mem)
                     *(LEAK_MEM_SIZE+LEAK_MEM_INCREMENT_SIZE)
            )
        );
        for(uint32_t i=LEAK_MEM_SIZE;i<LEAK_MEM_SIZE+LEAK_MEM_INCREMENT_SIZE;i++){
             memoryData.mem[i].address=0;
        }
        LEAK_MEM_SIZE+=LEAK_MEM_INCREMENT_SIZE;
        goto insert_to_mem;
#else
        _leak_warn(file,line,"LEAK_MEM_SIZE too low, allocate less memory or increase LEAK_MEM_SIZE");
        exit(EXIT_FAILURE);
#endif
    }
    return false;
}

/**
 * @return: 0 if success else -1
*/
static uint8_t _delete(void *ptr) {
    const size_t address = (size_t)ptr;

    if (ptr != NULL) {
        for (uint32_t i=0; i<LEAK_MEM_SIZE; i++) {
            if (address == memoryData.mem[i].address) {
                memoryData.mem[i].address = 0;

                memoryData.free++;
                memoryData.total_freed += memoryData.mem[i].size;
                return true;
            }
        }
    }

    memoryData.free++;
    return false;
}
typedef struct {
        uint32_t instances;
        char file[255];
        uint32_t line;
        size_t size;
    } memCollapser;
void show_memory_leak(memCollapser collapser) {
    
    printf("(%d)Memory leak at %s:%d ",
        collapser.instances+1,
        collapser.file,
        collapser.line
        );
    if(collapser.instances>0){
        printf("((%d)%zu bytes=%zu bytes)\n",
            collapser.instances+1,
            collapser.size,
            collapser.size*(size_t)(collapser.instances+1)
            );
    }else{
        printf("(%zu bytes)\n",collapser.size);
    }
}
void _generate_report() {
    printf("/*========= SUMMARY =========*/\n");
    printf("  Total allocations      %d  \n", memoryData.allocations);
    printf("  Total Free             %d  \n", memoryData.free);
    printf("  Total Memory allocated %zu bytes \n", memoryData.total_allocated);
    printf("  Total Memory freed     %zu bytes \n", memoryData.total_freed);
    printf("  Memory Leaked          %zu bytes \n", memoryData.total_allocated - memoryData.total_freed);

    if (memoryData.total_freed == memoryData.total_allocated) return;
    printf("\n/*===== DETAILED REPORT =====*/\n");
    memCollapser collapser;
    char j=0;
    for (uint32_t i=0; i<LEAK_MEM_SIZE; i++) {
        if (memoryData.mem[i].address != 0) {
            if(j==0){
                collapser.instances=0;
                strcpy(collapser.file,memoryData.mem[i].file);
                collapser.line=memoryData.mem[i].line;
                collapser.size=memoryData.mem[i].size;
                j=1;
            }
            else if(strcmp(collapser.file,memoryData.mem[i].file)==0 && collapser.line==memoryData.mem[i].line && collapser.size==memoryData.mem[i].size){
                collapser.instances+=1;
            }else{

                show_memory_leak(collapser);
                collapser.instances=0;
                strcpy(collapser.file,memoryData.mem[i].file);
                collapser.line=memoryData.mem[i].line;
                collapser.size=memoryData.mem[i].size;
        }
    }}
    if(j!=0){ 
        show_memory_leak(collapser);
    }
    printf("==============================\n");
}

void mem_at_exit() {
    _generate_report();
#ifdef LEAK_MEM_DYNAMIC
    free(memoryData.mem);
#endif
}
void init() {
    if (!initialized) {
        // printf("initializing...\n");
#ifdef LEAK_MEM_DYNAMIC
        memoryData.mem=(Mem*)mem_catch_alloc(malloc(sizeof(Mem)*LEAK_MEM_START_SIZE));
        for(int i=0;i<LEAK_MEM_START_SIZE;i++){
            memoryData.mem[i].address=0;
        }
#endif
        atexit(mem_at_exit);
        initialized = true;
    }
}

void *_malloc(size_t size, char *file, int line) {
    init();

    void *ptr = malloc(size);

    if (ptr == NULL) {
        // do something
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }

    _insert(ptr, size, line, file);
    return ptr;
}

void *_calloc(size_t num, size_t size, char *file, int line) {
    init();

    void *ptr = calloc(num, size);
    if (ptr == NULL) {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }

    _insert(ptr, num * size, line, file);
    return ptr;
}

void *_realloc(void *ptr, size_t size, char *file, int line) {
    if (ptr == NULL) {
        _leak_warn(file, line, "Tried to free a 'NULL' pointer");
    }
    if (!_delete(ptr)) {
        _leak_warn(file, line, "Double free detected");
        exit(EXIT_FAILURE);
    }
    void *new_ptr = realloc(ptr, size);

    if (new_ptr == NULL) {
        _leak_warn(file, line, "Memory allocation failed");
        return ptr;
    }


    _insert(new_ptr, size, line, file);

    return new_ptr;
}

void _free(void *ptr, char *file, int line) {
    if (ptr == NULL) {
        _leak_warn(file, line, "Tried to free a 'NULL' pointer");
    }
    if (!_delete(ptr)) {
        _leak_warn(file, line, "Double free detected");
        exit(EXIT_FAILURE);
    }
    
    free(ptr);
}

// Redefine allocator functions
#define malloc(size) _malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _calloc(num, size, __FILE__, __LINE__)
#define realloc(ptr, size) _realloc(ptr, size, __FILE__, __LINE__)
#define free(ptr) _free(ptr, __FILE__, __LINE__)
#define generate_report() _generate_report(__FILE__)

#ifdef __cpluscplus
}
#endif // __cpluscplus

#endif // __LEAK_DETECTOR_H_
