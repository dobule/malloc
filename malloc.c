/* Name: Ryan Gelston
 * Term: Fall 2018 (CSC 453)
 * Assignment: Assignment 1
 * Description: Includes implimentations of malloc, calloc, realloc, and free.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "malloc.h"
#include "print.h"

/* The smallest amount of memory one can allocate. All other block sizes
 * should be a multiple of this number. */
#define MIN_ALLOC 16

/* The size of sbrk requests are multiples of this */
#define BREAK_UNIT 65536

#define MAGIC 0x45C2

/* Memory block header */
typedef struct MB_Head MB_Head;
typedef struct MB_Head {
   MB_Head* prevBlock;/* Points to the first block with a lower address */
   MB_Head* nextFree; /* Points to the first free block with a higher addr */
   size_t size;       /* Adjusted to be a multiple of MIN_ALLOC */
   size_t freeMagic;  /* lsb determines if block is free, the rest acts as 
                          a magic number to identify the header */
} MB_Head;

typedef struct {
   MB_Head* bottom; /* Points to block with lowest memory address */
   MB_Head* top;    /* Points to a block with highest memory address */
   void* brkEnd;    /* The end of memory allocated by sbrk */
} Heap;


/***********************************************************
 * INTERNAL FUNCTION HEADERS
 **********************************************************/

/* Constructors for structs */
Heap heap_new();
MB_Head head_new(MB_Head* prevBlock, MB_Head* nextFree, int size);

/* Modify and find information about individual blocks */
int is_free(MB_Head* head);
void set_to_free(MB_Head* head);
void set_to_used(MB_Head* head);
int is_header(MB_Head* head);
int in_heap(void *ptr);
MB_Head* get_next_block(MB_Head *head);

/* Modify the list of blocks in the heap */
size_t align(size_t size, int minUnit);
void set_nextFree(MB_Head* startBlock, MB_Head* freeBlock);
MB_Head* create_new_block(size_t size);
MB_Head* find_free_block(size_t size);
MB_Head* resize_block(MB_Head *head, size_t newSize);
void merge_blocks(MB_Head* head);
MB_Head* seek_block(void* ptr);

/* Contain and manage heap */
Heap* get_heap();
void update_heap();
void* call_sbrk(size_t size);
void* increase_room(size_t size);
void* assure_room(size_t size);


/***********************************************************
 * CONSTRUCTORS FOR STRUCTS
 **********************************************************/

/* Creates a new heap */
Heap heap_new() {

   Heap heap;
   int remainder;

   dprint(2, "heap_new: Entering Function\n");

   heap.bottom = NULL;
   heap.top = NULL;

   heap.brkEnd = call_sbrk(0);

   remainder = (size_t)heap.brkEnd % MIN_ALLOC;

   /* Adjust the break point to be at an address in memory that is divisible
    * by MIN_ALLOC */
   if (remainder) {
      heap.brkEnd = call_sbrk(MIN_ALLOC - remainder);
   }
   return heap;
}


/* Creates new memory block header */
MB_Head head_new(MB_Head* prevBlock, MB_Head* nextFree, int size) {

   MB_Head head;

   dprint(2, "head_new: Entering Function\n");

   dprint_ptr(3, "head_new: nextFree set to", nextFree);

   head.prevBlock = prevBlock;
   head.nextFree = nextFree;
   head.size = size;

   /* NOTE: Magic var implicitly sets the block to be used*/
   head.freeMagic = MAGIC;

   return head;
}


/***********************************************************
 * Structure support functions 
 **********************************************************/

/* Returns 1 if a block header is free */
int is_free(MB_Head *head) {

   if (head == NULL) {
      return 0;
   }
   return head->freeMagic & 0x01;
}

/* Sets block to free */
void set_to_free(MB_Head *head) {
   if (head != NULL) {
      head->freeMagic = head->freeMagic | 0x01;
      set_nextFree(head, head);
   }
}

/* Sets block to used */
void set_to_used(MB_Head *head) {
   if (head != NULL) {
      head->freeMagic = head->freeMagic & ~0x01;
      set_nextFree(head, head->nextFree);
   }
}

/* Checks header for magic number */
int is_header(MB_Head *head) {
   return (~0x01 & head->freeMagic) == MAGIC;
}

/* Returns 1 if the passed pointer is in the heap */
int in_heap(void *ptr) {

   Heap *heap = get_heap();

   dprint(2, "in_heap: Entering Function\n");

   if ((void*)heap->bottom <= ptr 
        && ptr < (void*)((char*)heap->top 
                           + sizeof(MB_Head) 
                           + heap->top->size)) {
      return 1;
   }
   return 0;
}

/* Returns ptr to next block if next block is valid, NULL otherwise */
MB_Head* get_next_block(MB_Head *head) {
   
   MB_Head *next;

   dprint_ptr(2, "get_next_block: head val", head);

   if (head != NULL) {
      next = (MB_Head*)((char*)head + sizeof(MB_Head) + head->size);
      if (in_heap(next) && is_header(next)) {
         return next;
      }
   }
 
   return NULL;
}

/* Returns the first number greater than size 
 * that can be divided by minUnit */
size_t align(size_t size, int minUnit) {

   size_t toReturn;

   dprint(2, "align: Entering Function\n");

   dprint_int(3, "align: input size", size);

   if (size % minUnit == 0) {
      toReturn = size;
   } else {
      toReturn = size + minUnit - (size % minUnit);
   }

   dprint_int(3, "align: aligned size", toReturn);

   return toReturn;
}

/* Calls sbrk and performs error check */
void* call_sbrk(size_t size) {

   void *res = sbrk(size);

   dprint_int(3, "call_sbrk: Called with size", size);

   if (res == (void*)-1) {
      print("Call to sbrk failed\n");
      exit(-1);
   }

   return res;
}
    

/* Moves back from startHead and updates the nextFree values of blocks 
 * to freeBlock until but including the first encountered free block.
 * startHead is not updated in this funciton */
void set_nextFree(MB_Head* startHead, MB_Head* freeBlock) {

   MB_Head* head = startHead->prevBlock;

   dprint(2, "set_nextFree: Entering function\n");

   while (head != NULL) {
      head->nextFree = freeBlock;
       
      /* if a free block is found, stop updating nextFree values */
      if (is_free(head)) {
         head = NULL;
      } else {
         head = head->prevBlock;
      }
   }
}


/* Adds a block of a given size to the heap */
MB_Head* create_new_block(size_t size) {

   Heap *heap = get_heap();
   MB_Head *newTop;

   dprint(2, "create_new_block: Entering Function\n");

   /* Top and bottom of heap will be NULL if not initialized */
   if (heap->bottom == NULL) {
      newTop = heap->brkEnd;
      heap->bottom = newTop;
   } else {
      /* Cast to a char* to avoid multiples of sizeof(MB_Head) */
      newTop = (MB_Head*)((char*)heap->top
                            + sizeof(MB_Head)
                            + heap->top->size);
   }

   assure_room(sizeof(MB_Head) + size);
   *newTop = head_new(heap->top, NULL, size);
   dprint_ptr(2, "create_new_block: Top of heap set to", newTop);
   heap->top = newTop;

   return newTop;
}


/* Finds a free block in the heap that is greater than or equal to the passed
 * size. Returns NULL if nothing is found. */
MB_Head* find_free_block(size_t size) {

   Heap *heap = get_heap(); 
   MB_Head *head;

   dprint(2, "find_free_block: Entering function\n");
   dprint_ptr(3, "find_free_block: bottom of heap", heap->bottom);

   head = heap->bottom;

   /* Go to next free block */
   if (head != NULL && !is_free(head)) {
      head = head->nextFree;
      dprint_ptr(3, "find_free_block: nextFree val", head);
   }

   /* Iterate through the free list */
   while (head != NULL && head->size < size) {
      head = head->nextFree;
      dprint_ptr(3, "find_free_block: nextFree val", head);
   } 

   dprint_ptr(2, "find_free_block: head val", head);
   head = resize_block(head, size);
   dprint_ptr(2, "find_free_block: return val", head);

   return head;
}


/* Resizes a block to the passed size. It will attempt to expand or contract
 * the block in place before allocating a new block */
MB_Head* resize_block(MB_Head *head, size_t newSize) {

   Heap *heap = get_heap();
   size_t availableSpace;
   long int remainingSpace;
   MB_Head *remainingBlock, *nextBlock, *tmpNextFree;
   MB_Head *toReturn = head;

   if (head == NULL) {
      dprint(2, "resize_block: head is NULL\n");
      return NULL;
   }

   if (newSize == 0) {
      free((char*)head + sizeof(MB_Head));
      return NULL;
   }

   dprint(2, "resize_block: Entering Function\n");

   /* In the case that the block is on the top of the heap, the size of 
    * the block need only be changed. */
   /* NOTE: Return statement in this block */
   if (head == heap->top) {
      dprint(2, "resize_block: Extending the top of the heap\n");
      if (head->size < newSize) {
         assure_room(newSize - head->size);
      }
      head->size = newSize;
      return head;
   }

   availableSpace = head->size;

   nextBlock = get_next_block(head);
   tmpNextFree = NULL;
   if (nextBlock != NULL) {
      tmpNextFree = nextBlock->nextFree;
   } else {
      dprint(0, "THIS ALSO SHOULDN'T BE PRINTING\n");
      dprint_ptr(0, "resize_block: bottom of heap", heap->bottom);
      dprint_ptr(0, "resize_block: top of heap", heap->top);
      dprint_ptr(0, "resize_block: head", head);
      dprint_ptr(0, "resize_block: after head",
            (char*)head + sizeof(MB_Head) + head->size);
      exit(-1);
   }

   /* Update available space if there's a free block ahead. 
    * nextBlock and tmpNextFree must also be updated in this case*/
   if (is_free(nextBlock)) {
      availableSpace += sizeof(MB_Head) + nextBlock->size;
      nextBlock = get_next_block(nextBlock);
      tmpNextFree = NULL;
      if (nextBlock != NULL) {
         tmpNextFree = nextBlock->nextFree;
      } else {
         dprint(2, "THIS SHOULDN'T BE PRINTING\n");
         dprint_ptr(2, "resize_block: bottom of heap", heap->bottom);
         dprint_ptr(2, "resize_block: top of heap", heap->top);
         dprint_ptr(2, "resize_block: head", head);
         dprint_ptr(2, "resize_block: after head",
            (char*)head + sizeof(MB_Head) + head->size);
      }
   }

   remainingSpace = availableSpace - newSize;
   dprint_int(2, "resize_block: remaining space", remainingSpace);

   /* Extend the block or create a new block */
   if (remainingSpace < 0) {
      dprint(2, "resize_block: Need to create a new block\n");
      toReturn = malloc(newSize);
      toReturn = (void*)((char*)toReturn - sizeof(MB_Head));
      memcpy((char*)toReturn + sizeof(MB_Head),
             (char*)head + sizeof(MB_Head),
             head->size);
      free((char*)head + sizeof(MB_Head));
   } 

   /* If there is enough room, create a free block after head */
   if (remainingSpace >= (long int)(sizeof(MB_Head) + MIN_ALLOC)) {
      dprint(2, "resize_block: Creating new free block\n");
      remainingBlock = (MB_Head*)((char*)head + sizeof(MB_Head) + newSize);
      *remainingBlock = head_new(head, 
                                 tmpNextFree, 
                                 remainingSpace - sizeof(MB_Head));
      set_to_free(remainingBlock);
      dprint_ptr(3, "resize_block: setting nextFree", remainingBlock);
      head->size = newSize;
      if (nextBlock != NULL) {
         nextBlock->prevBlock = remainingBlock;
      }
   } 

   return toReturn;
}

/***********************************************************
 * free SUPPORT FUNCTIONS
 **********************************************************/

/* Merges the passed block and the next block if they are both free */
void merge_blocks(MB_Head *head) {

   MB_Head *next, *afterNext;

   /* Head ptr is invalid, return NULL */
   if (head == NULL) {
      dprint(2, "merge_blocks: passed NULL pointer\n");
      return;
   }

   dprint(2, "merge_blocks: Entering Function\n");

   next = get_next_block(head);
   afterNext = get_next_block(next);

   /* If both blocks are free, merge them into one block */
   if (is_free(head) && is_free(next)) {
      dprint(2, "merge_blocks: Merging Blocks\n");
      head->size = head->size + sizeof(MB_Head) + next->size;
      head->nextFree = next->nextFree;
      /* Update the block after the merged block */
      if (afterNext != NULL) {
         afterNext->prevBlock = head;
         dprint_ptr(3, "merge_blocks: prevBlock set to", 
                    afterNext->prevBlock);
      }
   }
}


/* Finds the block that the pointer is located in */
MB_Head* seek_block(void* ptr) {

   Heap *heap = get_heap();
   MB_Head *cur;

   dprint(2, "seek_block: Entering Function\n");

   /* Return null if ptr is before the heap */
   if (ptr < (void*)heap->bottom) {
      return NULL;
   }

   cur = heap->bottom;

   /* Keep going until there are no more blocks or ptr is before 
    * the end of the current block */
   while (cur != NULL
           && (void*)((char*)cur + sizeof(MB_Head) + cur->size) <= ptr) {
      cur = get_next_block(cur);
   }

   return cur;
}


/* Stores and returns a pointer to the heap */
Heap* get_heap() {
   
   static int heapCreated = 0;
   static Heap heap;

   if (!heapCreated) {
      heap = heap_new();
      heapCreated = 1;
   }

   return &heap;
}

/* Looks for free block at the top of the heap and reliquishes
 * unused space at the top of the heap back to the OS */
void update_heap() {

   Heap *heap = get_heap();
   MB_Head *tmpHead = heap->top;
   size_t openMemory, breakAmount;

   dprint(2, "update_heap: Entering Function\n");

   /* Set the top of the heap to be the top-most used block */
   while (is_free(tmpHead)) {
      tmpHead = tmpHead->prevBlock;
   }

   heap->top = tmpHead;

   /* Update nextFree values if applicable */
   if (heap->top != NULL) {
      heap->top->nextFree = NULL;
      set_nextFree(heap->top, NULL);
   } 

   /* Relinquish memory back to the OS if possible */
   if (heap->top != NULL) {
      openMemory = (char*)heap->brkEnd
                     - (char*)heap->top
                     - sizeof(MB_Head)
                     - heap->top->size;
      breakAmount = (openMemory / BREAK_UNIT) * BREAK_UNIT;
      /* No need to call sbrk if it won't adjust heap space */
      if (breakAmount != 0) {
         call_sbrk(-1 * breakAmount);
         heap->brkEnd = (char*)heap->brkEnd - breakAmount;
      }
   /* If the top is NULL, the heap is empty and we can reliquish all memory */
   } else {
      call_sbrk((char*)heap->bottom - (char*)heap->brkEnd);
      heap->brkEnd = (void*)heap->bottom;
      heap->bottom = NULL;
   }
}

/* Increases the amount of available memory */
void* increase_room(size_t size) {

   Heap *heap = get_heap();

   dprint(2, "increase_room: Entering Function\n");

   /* Finds closest multiple of BREAK_UNIT that size can fit in */
   int alignedSize = align(size, BREAK_UNIT);

   heap->brkEnd = (void*)((char*)heap->brkEnd + alignedSize);
   dprint_ptr(3, "increase_room: brkEnd set to\n", heap->brkEnd);
   return call_sbrk(alignedSize);
}


/* Checks if there is room on the heap for another memory block of the given
 * size. If there is not enough room, expand the heap accordingly */
void* assure_room(size_t size) {

   void *newBlockEnd;
   Heap *heap = get_heap();

   dprint(2, "assure_room: Entering Function\n"); 
  
   /* Test the top of the heap, as the bottom is reset in 
    * create_new_block */
   if (heap->top == NULL) {
      dprint(3, "assure_room: Bottom of heap is NULL\n");
      return increase_room(size);
   } else {
      newBlockEnd = (char*)heap->top
                     + sizeof(MB_Head)
                     + heap->top->size
                     + sizeof(MB_Head)
                     + size;
      if (newBlockEnd > heap->brkEnd) {
         return increase_room((size_t)((char*)newBlockEnd 
                                         - (char*)heap->brkEnd));
      }
   }

   return NULL;
}
       


/***********************************************************
 * Functions in header file
 **********************************************************/

void* malloc(size_t size) {

   void *head;
   size_t newSize;

   dprint(2, "malloc: Entering Function\n");

   if (size == 0) {
      malloc_print(0, NULL, 0);
      return NULL;
   }

   newSize = align(size, MIN_ALLOC);
   head = find_free_block(newSize);

   if (head == NULL) {
      head = create_new_block(newSize);
   }

   set_to_used(head);

   malloc_print(size, (void*)((char*)head + sizeof(MB_Head)), newSize);
   return (void*)((char*)head + sizeof(MB_Head));
}


void* calloc(size_t num, size_t size) {

   void *toReturn;
   size_t adjSize = align(num * size, MIN_ALLOC);
   int i;

   dprint(2, "calloc: Entering Function\n");

   toReturn = malloc(adjSize);

   for (i = 0; i < adjSize; i++) {
      ((char*)toReturn)[i] = 0;
   }

   calloc_print(num, size, toReturn, adjSize);

   return toReturn;
}


void* realloc(void* ptr, size_t size) {

   void *toReturn; 
   size_t adjSize = align(size, MIN_ALLOC);
   MB_Head *head = (MB_Head*)((char*)ptr - sizeof(MB_Head));

   dprint(2, "realloc: Entering function\n");

   toReturn = resize_block(head, adjSize);

   /* Don't incriment past header if toReturn is NULL */
   if (toReturn) {
      toReturn = (void*)((char*)toReturn + sizeof(MB_Head));
   }

   realloc_print(ptr, size, toReturn, adjSize); 

   return toReturn;
}


void free(void *ptr) {

   Heap *heap = get_heap();
   MB_Head *head = (MB_Head*)((char*)ptr - sizeof(MB_Head));

   dprint(2, "free: Entering Function\n");

   dprint_ptr(2, "free: passed ptr", ptr);
   dprint_ptr(2, "free: bottom of heap", heap->bottom);
   dprint_ptr(2, "free: top of heap", heap->top);

   /* Check to see if heap is empty */
   if (heap->bottom == NULL) {
      dprint(0, "free: Heap is empty\n");
      free_print(ptr);
      return;
   }

   /* Check if the ptr is in the heap */
   if (!in_heap(ptr)) {
      dprint(0, "free: Address not in heap, nothing is freed\n");
      free_print(ptr);
      return;
   }

   /* If the supposed header is not in the heap or is not a header,
    * seek out the block where ptr is in */
   if (!(in_heap(head) && is_header(head))) {
      dprint(0, "free: Seeking to block\n");
      head = seek_block(ptr); 
   }

   /* Sets the free bit in the block and updates nextFree in prior blocks */
   set_to_free(head);

   /* Merges this free block with surrounding free blocks */
   merge_blocks(head);
   merge_blocks(head->prevBlock);

   /* Remove and free blocks at the top of the heap and reliquish
    * memory to the OS if possible */
   update_heap();

   free_print(ptr);
}
