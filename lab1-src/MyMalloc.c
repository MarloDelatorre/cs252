/*
 * CS252: MyMalloc Project
 *
 * The current implementation gets memory from the OS
 * every time memory is requested and never frees memory.
 *
 * You will implement the allocator as indicated in the handout,
 * as well as the deallocator.
 *
 * You will also need to add the necessary locking mechanisms to
 * support multi-threaded programs.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int arenaSize = 2097152;

void increaseMallocCalls()  { _mallocCalls++; }

void increaseReallocCalls() { _reallocCalls++; }

void increaseCallocCalls()  { _callocCalls++; }

void increaseFreeCalls()    { _freeCalls++; }

extern void atExitHandlerInC()
{
    atExitHandler();
}

/* 
 * Initial setup of allocator. First chunk is retrieved from the OS,
 * and the fence posts and freeList are initialized.
 */
void initialize()
{
    // Environment var VERBOSE prints stats at end and turns on debugging
    // Default is on
    _verbose = 1;
    const char *envverbose = getenv("MALLOCVERBOSE");
    if (envverbose && !strcmp(envverbose, "NO")) {
        _verbose = 0;
    }

    pthread_mutex_init(&mutex, NULL);
    void *_mem = getMemoryFromOS(arenaSize);

    // In verbose mode register also printing statistics at exit
    atexit(atExitHandlerInC);

    // establish fence posts
    ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
    fencePostHead->_allocated = 1;
    fencePostHead->_objectSize = 0;

    char *temp = (char *)_mem + arenaSize - sizeof(ObjectHeader);
    ObjectHeader * fencePostFoot = (ObjectHeader *)temp;
    fencePostFoot->_allocated = 1;
    fencePostFoot->_objectSize = 0;

    // Set up the sentinel as the start of the freeList
    _freeList = &_freeListSentinel;

    // Initialize the list to point to the _mem
    temp = (char *)_mem + sizeof(ObjectHeader);
    ObjectHeader *currentHeader = (ObjectHeader *)temp;
    currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader)); // ~2MB
    currentHeader->_leftObjectSize = 0;
    currentHeader->_allocated = 0;
    currentHeader->_listNext = _freeList;
    currentHeader->_listPrev = _freeList;
    _freeList->_listNext = currentHeader;
    _freeList->_listPrev = currentHeader;

    // Set the start of the allocated memory
    _memStart = (char *)currentHeader;

    _initialized = 1;
}

/* 
 * @param: amount of memory requested
 * @return: pointer to start of useable memory
 */
void * allocateObject(size_t size)
{
    // Make sure that allocator is initialized
    if (!_initialized)
        initialize();

    /* Add the ObjectHeader to the size and round the total size up to a 
     * multiple of 8 bytes for alignment.
     */
    size_t roundedSize = (size + sizeof(ObjectHeader) + 7) & ~7;
    
    // fl_search will traverse the freelist and return a pointer to the first
    // header which has enough memory for roundedSize; return NULL if we don't find
    // a header with sufficient memory.
    ObjectHeader * memChunk = fl_search(roundedSize);

    // If it turns out that fl_search could not find a sufficient header, we call
    // fl_insert which will add a block to the beginning of the freelist and return
    // a pointer to it.
    if (memChunk == NULL) {
      memChunk = fl_create();
    }
    
    assert(memChunk != NULL);

    // We have now guaranteed that memChunk points to a header with memory that is 
    // greater or equal to roundedSize. Now we should check if we should split the chunk 
    // of memory or not. (i.e. after splitting do we have enough space for a header 
    // + 8 bytes?)
    ObjectHeader * _mem = NULL;
    if (memChunk->_objectSize - roundedSize >= sizeof(ObjectHeader) + 8) {
      // We have enough space for another memory chunk, split the block 
      _mem = split_chunk(memChunk, roundedSize); 
    } else {
      // We didn't have enough space for another memory chunk, don't split the block
      // Remove the chunk from the freelist
      _mem = memChunk;
      fl_remove(memChunk);
    }

    assert(_mem != NULL);
    
    _mem->_allocated = 1;
    // _mem now contains a pointer to the header of the memory we want to allocated,
    // we have asserted that it does not point to NULL

    pthread_mutex_unlock(&mutex);

    // Return a pointer to useable memory
    return (void *)((char *)_mem + sizeof(ObjectHeader));
}

/* 
 * @param: pointer to the beginning of the block to be returned
 * Note: ptr points to beginning of useable memory, not the block's header
 */
void freeObject(void *ptr)
{
    ObjectHeader * centerHeader = (ObjectHeader *)((char *)ptr - sizeof(ObjectHeader));
    centerHeader->_allocated = 0;

    // Check if right header is alloc'd, if so absorb it into center
    ObjectHeader * rightHeader = (ObjectHeader *)((char *)centerHeader + centerHeader->_objectSize);
    
    if (!rightHeader->_allocated) {
      // Remove rightHeader from freelist
      fl_remove(rightHeader);

      // Update centerHeader's fields
      centerHeader->_objectSize += rightHeader->_objectSize;
      
      // Update header right of the rightHeader (if it's not a dummy)
      ObjectHeader * farRightHeader = (ObjectHeader *)((char*)rightHeader + rightHeader->_objectSize);
      if (farRightHeader->_objectSize) {
        farRightHeader->_leftObjectSize = centerHeader->_objectSize;
      }
    }

    // Check if left header is alloc'd, if so absorb center into left
    ObjectHeader * leftHeader = (ObjectHeader *)((char *)centerHeader - centerHeader->_leftObjectSize);
    if (!leftHeader->_allocated) {
      // Update leftHeader's fields
      leftHeader->_objectSize += centerHeader->_objectSize;

      // Update header right of leftHeader (if it's not a dummy)
      ObjectHeader * farRightHeader = (ObjectHeader *)((char *)centerHeader + centerHeader->_objectSize);
      if (farRightHeader->_objectSize) {
        farRightHeader->_leftObjectSize = leftHeader->_objectSize;
      }
    }
    // Otherwise, insert center into freelist
    else {
      fl_insert(centerHeader);
    }
    return;
}

/* 
 * Prints the current state of the heap.
 */
void print()
{
    printf("\n-------------------\n");

    printf("HeapSize:\t%zd bytes\n", _heapSize );
    printf("# mallocs:\t%d\n", _mallocCalls );
    printf("# reallocs:\t%d\n", _reallocCalls );
    printf("# callocs:\t%d\n", _callocCalls );
    printf("# frees:\t%d\n", _freeCalls );

    printf("\n-------------------\n");
}

/* 
 * Prints the current state of the freeList
 */
void print_list() {
    printf("FreeList: ");
    if (!_initialized) 
        initialize();

    ObjectHeader * ptr = _freeList->_listNext;

    while (ptr != _freeList) {
        long offset = (long)ptr - (long)_memStart;
        printf("[offset:%ld,size:%zd]", offset, ptr->_objectSize);
        ptr = ptr->_listNext;
        if (ptr != NULL)
            printf("->");
    }
    printf("\n");
}

/* 
 * This function employs the actual system call, sbrk, that retrieves memory
 * from the OS.
 *
 * @param: the chunk size that is requested from the OS
 * @return: pointer to the beginning of the chunk retrieved from the OS
 */
void * getMemoryFromOS(size_t size)
{
    _heapSize += size;

    // Use sbrk() to get memory from OS
    void *_mem = sbrk(size);

    // if the list hasn't been initialized, initialize memStart to mem
    if (!_initialized)
        _memStart = _mem;

    return _mem;
}

void atExitHandler()
{
    // Print statistics when exit
    if (_verbose)
        print();
}

/*
 * C interface
 */

extern void * malloc(size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseMallocCalls();

    return allocateObject(size);
}

extern void free(void *ptr)
{
    pthread_mutex_lock(&mutex);
    increaseFreeCalls();

    if (ptr == 0) {
        // No object to free
        pthread_mutex_unlock(&mutex);
        return;
    }

    freeObject(ptr);
}

extern void * realloc(void *ptr, size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseReallocCalls();

    // Allocate new object
    void *newptr = allocateObject(size);

    // Copy old object only if ptr != 0
    if (ptr != 0) {

        // copy only the minimum number of bytes
        ObjectHeader* hdr = (ObjectHeader *)((char *) ptr - sizeof(ObjectHeader));
        size_t sizeToCopy =  hdr->_objectSize;
        if (sizeToCopy > size)
            sizeToCopy = size;

        memcpy(newptr, ptr, sizeToCopy);

        //Free old object
        freeObject(ptr);
    }

    return newptr;
}

extern void * calloc(size_t nelem, size_t elsize)
{
    pthread_mutex_lock(&mutex);
    increaseCallocCalls();

    // calloc allocates and initializes
    size_t size = nelem *elsize;

    void *ptr = allocateObject(size);

    if (ptr) {
        // No error; initialize chunk with 0s
        memset(ptr, 0, size);
    }

    return ptr;
}

// Auxilary functions for allocateObject(..)
ObjectHeader * fl_search(size_t size) {
  ObjectHeader * curr = _freeList->_listNext;
  while (curr != &_freeListSentinel) {
    if (curr->_objectSize >= size) {
      return curr;
    }
    curr = curr->_listNext;
  }
  return NULL;
}

ObjectHeader * fl_create() {
  // Get memory from OS
  void *_mem = getMemoryFromOS(arenaSize);

  // Write fenceposts
  ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
  fencePostHead->_allocated = 1;
  fencePostHead->_objectSize = 0;

  char * temp = (char *)_mem + arenaSize - sizeof(ObjectHeader);
  ObjectHeader * fencePostTail = (ObjectHeader *)temp;
  fencePostTail->_allocated = 1;
  fencePostTail->_objectSize = 0;

  // Write header
  temp = (char *)_mem + sizeof(ObjectHeader);
  ObjectHeader *currentHeader = (ObjectHeader *)temp;
  currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader));
  currentHeader->_leftObjectSize = 0;
  currentHeader->_allocated = 0;
  
  // Insert into beginning of freelist
  currentHeader->_listNext = _freeList->_listNext;
  currentHeader->_listPrev = _freeList;
  _freeList->_listNext = currentHeader;
  
  return currentHeader;
}

ObjectHeader * split_chunk(ObjectHeader * header, size_t size) {
  // Calculate position of next header
  size_t leftoverMem = header->_objectSize - size;

  // Write fields to new header
  ObjectHeader * newHeader = (ObjectHeader *)((char *)header + leftoverMem);
  newHeader->_objectSize = size;
  newHeader->_leftObjectSize = leftoverMem;
  newHeader->_allocated = 0;
  newHeader->_listNext = NULL;
  newHeader->_listPrev = NULL;

  // Update the header to the left
  header->_objectSize = leftoverMem;
  
  // Update the header to the right (if it's not a dummy header)
  ObjectHeader * rightHeader = (ObjectHeader *)((char *)newHeader + newHeader->_objectSize);
  if (rightHeader->_objectSize) {
    rightHeader->_leftObjectSize = newHeader->_objectSize;
  }

  return newHeader;
}

void fl_remove(ObjectHeader * header) {
  // Check to see if the chunk is actually in the freelist
  if (header->_listPrev == NULL || header->_listNext == NULL) {
    return;
  }
  header->_listPrev->_listNext = header->_listNext;
  header->_listNext->_listPrev = header->_listPrev;
  header->_listPrev = NULL;
  header->_listNext = NULL;
}

void fl_insert(ObjectHeader * header) {
  _freeList->_listNext->_listPrev = header;
  header->_listNext = _freeList->_listNext;
  header->_listPrev = _freeList;
  _freeList->_listNext = header;
}
