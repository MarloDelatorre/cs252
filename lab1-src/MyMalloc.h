/*
 * CS252: MyMalloc header file
 *
 * The various variables, functions, and structs associated
 * with the allocator are defined here.
 */

// Header of an object. Used both when the object is allocated and freed
typedef struct ObjectHeader {
    size_t _objectSize;             // Real size of the object.
    int _leftObjectSize;            // Real size of the previous contiguous chunk in memory
    int _allocated;                 // 1 = yes, 0 = no.
    struct ObjectHeader *_listNext; // Points to the next object in the freelist (if free).
    struct ObjectHeader *_listPrev; // Points to the previous object.
} ObjectHeader;

// STATE of the allocator

size_t _heapSize;     // Size of the heap

void *_memStart;      // initial memory pool

int _initialized;     // True if heap has been initialized

int _verbose;         // Verbose mode

int _mallocCalls;     // # malloc calls

int _freeCalls;       // # free calls

int _reallocCalls;    // # realloc calls

int _callocCalls;     // # realloc calls

ObjectHeader *_freeList;          // Free list

ObjectHeader _freeListSentinel;   // Sentinel of free list

//FUNCTIONS

void initialize(); //Initializes the heap

void * allocateObject(size_t size); // Allocates an object 

void freeObject(void *ptr);         // Frees an object

size_t objectSize(void *ptr);       // Returns the size of an object

void atExitHandler();               // At exit handler

void print();         // Prints the current information about the allocator

void print_list();    // Prints the current state of the free list

void * getMemoryFromOS(size_t size); // Gets memory from the OS

// Auxilary functions for allocateObject(..)

// Traverses the freelist to return a pointer to a header that contains sufficient size, returns null otherwise
ObjectHeader * fl_search(size_t size);

// Requests 2mb memory from the OS, sets up fenceposts and initial header, and returns a pointer to the initial header. As a side effect, it inserts that header to the beginning of the freelist
ObjectHeader * fl_insert();

// Splits a chunk of memory. Establishes a new header to the right of chunk, updates its and chunk's fields, and returns a pointer to the new header.
ObjectHeader * split_chunk(ObjectHeader * chunk, size_t size);

// Removes an ObjectHeader from the freelist
void fl_remove(ObjectHeader * chunk);
