/* Name: Ryan Gelston
 * Filename: malloc_test.c
 * Description: Contains tests for the functions malloc, calloc, realloc, and
 *    free.
 */

#include <unistd.h>
/*#include "malloc.h" */
#include <stdlib.h>
#include "print.h"

/* This is only for 64-bit systems */
#define HEAD_SIZE 32
#define BREAK_UNIT 65536

#define NUM_BLOCKS 4


/**********************************************************
 * Test Function Declarations 
 *********************************************************/
void test_print();
void test_malloc_separation();
void test_join_free_blocks();
void test_block_isolation();
void test_block_division();
void test_call_to_sbrk();
void test_seek_free_block();
void test_free_garbage();
void test_malloc_garbage();
void write_over_block();
void test_huge_malloc();
void test_calloc(size_t size);
void test_random_malloc(int numItr, int maxBlockSize);
void test_grow_realloc();
void test_grow_realloc2();
void test_extend_top_realloc();
void test_shrink_realloc();
void test_random_realloc(int numItr, int maxBlockSize);


int main(int argc, char* argv[]) {

   test_print();
   test_malloc_separation();
   test_join_free_blocks(); 
   test_block_isolation(); 
   test_block_division();
   test_call_to_sbrk();
   test_seek_free_block();
   test_free_garbage(); 
   test_malloc_garbage(); 
   test_calloc(64);
   test_calloc(20);
   test_huge_malloc(); 
   test_random_malloc(1000000, BREAK_UNIT);  
   test_grow_realloc(); 
   test_grow_realloc2();
   test_extend_top_realloc();
   test_shrink_realloc();
   test_random_realloc(1000000, 32);
   
   return 0;
}

void test_print() {
   
   /* Thing to point to */
   int ttpt = 42;

   print("PRINTING TESTS\n");
   print("You should always see this\n");
   dprint(1, "Debug print level 1\n");
   dprint(2, "Debug print level 2\n");
   dprint(3, "Debug print level 3\n");
   dprint(4, "Debug print level 4\n");
   dprint(5, "Debug print level 5\n");
   dprint_ptr(1, "Ptr, level 1", &ttpt);
   dprint_int(1, "Pi, level 1", 31415);
   malloc_print(42, &ttpt, 42);
   calloc_print(42, 42, &ttpt, 42);
   realloc_print(&ttpt, 42, &ttpt, 42);
   free_print(&ttpt);
   print("END PRINTING TESTS\n");

}

void test_malloc_separation() {

   print("\nTEST: BLOCK SEPARATION\n");

   void* res = malloc(4);
   void* res2 = malloc(5);

   free(res);
   free(res2);

   if (res2 != res) {
      print("PASSED: Blocks are separate\n");
   } else {
      print("FAILED: They are the same block\n");
   }

}

/* See if two free blocks joined */
void test_join_free_blocks() {

   void *res1, *res2, *res3, *res4, *res5;

   print("\nTEST: JOIN TWO FREE BLOCKS\n");

   res1 = malloc(16);
   res2 = malloc(32);
   res3 = malloc(16);
   res4 = malloc(16);

   free(res2);
   free(res3);

   res5 = malloc(48);

   free(res1);
   free(res4);
   free(res5);

   print("Test\n");

   if (res2 == res5) {
      print("PASSED: Free blocks were joined\n");
   } else {
      print("FAILED: Free blocks were not joined\n");
      dprint_ptr(0, "Expected", res2);
      dprint_ptr(0, "Actual", res5);
 
   }
}


/* Try to write over the next header without going outside of the block */
void test_block_isolation() {

   int i;
   char *res1, *res2, *res3, *res4;

   print("\nTEST: BLOCK SIZE VALIDATION\n");

   res1 = malloc(16);
   res2 = malloc(16);
   res3 = malloc(16);

   free(res2);

   for (i=0; i<16; i++) {
      res1[i] = 0xF; 
   }
   
   res4 = malloc(16);

   free(res1);
   free(res3);
   free(res4);

   if (res2 == res4) {
      print("PASSED: Did not write over header\n");
   } else {
      print("FAILED: Somehow wrote over the header\n");
   }
}

void test_block_division() {

   void *res1, *res2, *res3, *res4;

   print("\nTEST: BLOCK DIVISION\n");

   /* Note that without creating a block after res1, the res1 block would be
    * removed from the list after freeing */
   res1 = malloc(4 * HEAD_SIZE);
   res2 = malloc(12);
   free(res1);

   res3 = malloc(HEAD_SIZE);
   res4 = malloc(HEAD_SIZE);

   if (res3 == res1 && res4 < res2 && res4 > res1) {
      print("PASSED: Blocks were written inside free block\n");
   } else {
      print("FAILED: Block(s) were written outside of free block\n");
      dprint_ptr(0, "Free block", res1);
      dprint_ptr(0, "First inner block", res3);
      dprint_ptr(0, "Second inner block", res4);
   }

   free(res2);
   free(res3);
   free(res4);
}

void test_call_to_sbrk() {

   void *res1, *res2, *res3;
   print("\nTEST: CALL TO SBRK\n");

   res1 = sbrk(0);

   res2 = malloc(3 * BREAK_UNIT);

   res3 = sbrk(0);

   if (res3 - res1 == 4 * BREAK_UNIT) {
      print("PASSED: Sbrk increased expected amount\n");
   } else {
      print("FAILED: Sbrk did not increase by expected amount\n");
      dprint_ptr(0, "Start addr", res1);
      dprint_ptr(0, "End addr", res3);
      dprint_int(0, "Expected", 4 * BREAK_UNIT);
      dprint_int(0, "Actual", (int)(res3 - res1));
   }

   free(res2);
}

void test_seek_free_block() {

   void *res1, *res2, *res3;
   print("\nTEST: SEEK FREE BLOCK\n");

   res1 = malloc(16);
   res2 = malloc(5*BREAK_UNIT + 28);
   res3 = malloc(300);

   free(res3 + 45);
   free(res2 + 14000);
   free(res1 + 15);

   print("PASSED: Seek Free Block\n");
}

/* Pass garbage values to free */
void test_free_garbage() {
   
   void *res;

   print("\nTEST FREE GARBAGE\n");

   res = malloc(12);

   free(NULL);             /* Clearly not in the heap */
   free((char*)res - 50);  /* Below the only block */
   free((char*)res + 100000); /* Way above the only block */

   free(res);

   print("PASSED: No segfaults! :D\n");

}

/* Pass garbage values to malloc */
void test_malloc_garbage() {

   void *res;

   print("\nTEST MALLOC GARBAGE\n");

   res = malloc(0);

   if (res != NULL) {
      dprint_ptr(0, "FAILED: malloc(0) returned\n", res);
   } else {
      print("PASSED: malloc(0) returned NULL\n"); 
   }
}

void write_over_block(void* ptr, int size) {

   int i;

   for (i = 0; i < size; i++) {
      ((char*)ptr)[i] = 0;
   } 

   return;
}

void test_huge_malloc() {

   void *brkStart, *brkEnd;
   void *res1, *res2, *res3, *res4, *res5;

   print("\nTEST HUGE MALLOC\n");

   brkStart = sbrk(0);

   res1 = malloc(13412);
   res2 = malloc(109222);
   res3 = malloc(871117109);
   res4 = malloc(1000000000);
   res5 = malloc(7103563);

   write_over_block(res1, 13412);
   write_over_block(res2, 109222);
   write_over_block(res3, 871117109);
   write_over_block(res4, 1000000000);
   write_over_block(res5, 7103563);

   free(res2);
   free(res4);
   free(res3);
   free(res1);
   free(res5);

   brkEnd = sbrk(0);

   if (brkStart == brkEnd) {
      print("PASSED: All memory blocks recovered\n");
   } else {
      print("FAILED: Sbrk is not back to normal\n");
      dprint_ptr(0, "Start brk ptr", brkStart);
      dprint_ptr(0, "End brk ptr", brkEnd);
   }

}

void test_calloc(size_t size) {

   char *res = calloc(size, 1);
   int i;

   print("\nTEST CALLOC\n");

   for (i = 0; i < size; i++) {
      if (res[i] != 0) {
         print("FAILED: Part of the mem block is not set to zero\n");
         return;
      }
   } 

   free(res);

   print("PASSED: Everything is set to zero!!!\n");
}

/* Mallocs and frees numItr blocks fo memory. */
void test_random_malloc(int numItr, int maxBlockSize) {

   int i, idx;
   size_t size;
   int isFree[NUM_BLOCKS]; 
   void *res[NUM_BLOCKS];
   void *brkStart = sbrk(0);
   void *brkEnd;

   print("\nRANDOM MALLOC TEST\n");

   /* All blocks start unalocated */
   for (i = 0; i < NUM_BLOCKS; i++) {
      isFree[i] = 1;
   }

   /* Randomly allocate and free numItr blocks of memory */
   for (i = 0; i < numItr; i++) {
      idx = rand() % NUM_BLOCKS;
      size = rand();
      if (isFree[idx]) {
         size = rand() % maxBlockSize;
         res[idx] = malloc(size);
         if (res[idx] != NULL) {
            isFree[idx] = 0;
            write_over_block(res[idx], size);
         }
      } else {
         free(res[idx]);
         isFree[idx] = 1;
      }
   }

   print("FREEING REMAINING ALLOCATED BLOCKS\n");

   /* Free all remaining allocated blocks */
   for (i = 0; i < NUM_BLOCKS; i++) {
      if (!isFree[i]) {
         free(res[i]);
         isFree[idx] = 0;
      }
   }

   brkEnd = sbrk(0);

   if ((char*)brkEnd - (char*)brkStart >=16) {
      print("FAILED: brk is not back to the beginning\n");
      dprint_ptr(0, "Starting brk val", brkStart);
      dprint_ptr(0, "End brk val", brkEnd);
   } else {
      print("PASSED: No segfaults :D\n");
   }
      
   dprint_int(0, "Diff", (int)((char*)brkEnd - (char*)brkStart));
}


void test_grow_realloc() {

   int passed = 1;
   void *brkStart, *brkEnd;
   void *res1, *res2, *res3, *res4, *res5, *res6;

   print("\nTEST GROW REALLOC\n");

   brkStart = sbrk(0);

   res1 = malloc(16);
   res2 = malloc(16);
   res3 = malloc(16);
   write_over_block(res1, 16);
   write_over_block(res2, 16);
   write_over_block(res3, 16);
   res4 = realloc(res2, 32);
   res5 = malloc(16);
   write_over_block(res4, 32);
   write_over_block(res5, 16);
   res6 = realloc(res4, 48);
   write_over_block(res6, 48);

   free(res1);
   free(res3);
   free(res6);
   free(res5);

   brkEnd = sbrk(0);

   if (res2 != res5) {
      passed = 0;
      print("FAILED: Did not malloc into freed space\n");
   }

   if (brkStart != brkEnd) {
      passed = 0;
      print("FAILED: brk is not reset to the beginning\n");
   }

   if (passed) {
      print("PASSED: Realloc extended the top of the stack\n");
   }

}


void test_grow_realloc2() {

   int passed = 1;
   void *brkStart, *brkEnd;
   void *res1, *res2, *res3, *res4;

   print("\nTEST GROW REALLOC 2\n");

   brkStart = sbrk(0);

   res1 = malloc(16);
   res2 = malloc(BREAK_UNIT - 200);
   res3 = malloc(16);
   res4 = realloc(res3, 200);

   free(res4);
   free(res2);
   free(res1);

   brkEnd = sbrk(0);

   if (brkStart != brkEnd) {
      passed = 0;
      print("FAILED: brk is not reset to the beginning\n");
   }

   if (passed) {
      print("PASSED: Realloc extended the top of the stack\n");
   }
}


void test_extend_top_realloc() {

   int passed = 1;
   void *brkStart, *brkEnd;
   void *res1, *res2, *res3, *res4;

   print("\nTEST EXTEND TOP REALLOC\n");

   brkStart = sbrk(0);

   res1 = malloc(16);
   res2 = malloc(32);
   res3 = realloc(res2, 48);

   write_over_block(res3, 48);

   free(res1);
   free(res3);

   brkEnd = sbrk(0);

   if (res2 != res3) {
      passed = 0;
      print("FAILED: Realloc call did not extend top of stack\n");
      dprint_ptr(0, "Origional ptr", res2);
      dprint_ptr(0, "Realloced ptr", res3);
   } 

   if (brkStart != brkEnd) {
      passed = 0;
      print("FAILED: brk is not reset to the beginning\n");
   }

   if (passed) {
      print("PASSED: Realloc extended the top of the stack\n");
   }
}

void test_shrink_realloc() {

   int passed = 1;
   void *brkStart, *brkEnd;
   void *res1, *res2, *res3, *res4, *res5;

   print("\nTEST SHRINK REALLOC\n");

   brkStart = sbrk(0);

   res1 = malloc(16);
   res2 = malloc(3 * HEAD_SIZE);
   res3 = malloc(16);
   write_over_block(res2, 3 * HEAD_SIZE);
   res4 = realloc(res2, HEAD_SIZE);
   res5 = malloc(HEAD_SIZE);

   write_over_block(res1, 16);
   write_over_block(res4, HEAD_SIZE);
   write_over_block(res5, HEAD_SIZE);

   free(res1);
   free(res3);
   free(res4);
   free(res5);

   brkEnd = sbrk(0);

   if (res2 != res4) {
      passed = 0;
      print("FAILED: Realloc allocated another block\n");
   }

   if (res5 <= res1 || res5 >= res3) {
      passed = 0;
      print("FAILED: Free block not created as a result of shrinkage\n");
   }

   if (brkStart != brkEnd) {
      passed = 0;
      print("FAILED: Brk didn't return to it's origional location\n");
   }

   if (passed) {
      print("PASSED: Realloc shrank a block and created a free block\n");
   }
}

/* Mallocs and frees numItr blocks fo memory. */
void test_random_realloc(int numItr, int maxBlockSize) {

   /* Choice decides whether to free or to realloc */
   int i, idx, choice;
   size_t size;
   int isFree[NUM_BLOCKS]; 
   void *res[NUM_BLOCKS];
   void *brkStart = sbrk(0);
   void *brkEnd;
   

   print("\nRANDOM REALLOC TEST\n");

   /* All blocks start unalocated */
   for (i = 0; i < NUM_BLOCKS; i++) {
      isFree[i] = 1;
   }

   /* Randomly allocate and free numItr blocks of memory */
   for (i = 0; i < numItr; i++) {
      idx = rand() % NUM_BLOCKS;
      if (isFree[idx]) {
         size = rand() % maxBlockSize;
         res[idx] = malloc(size);
         if (res[idx] != NULL) {
            isFree[idx] = 0;
            write_over_block(res[idx], size);
         }
      } else {
         choice = rand() % 2;
         if (choice) {
            size = rand() % maxBlockSize;
            res[idx] = realloc(res[idx], size);
            if (res[idx] != NULL) {
               isFree[idx] = 0;
               write_over_block(res[idx], size);
            }
         } else {
            free(res[idx]);
            isFree[idx] = 1;
         }
      }
   }

   print("FREEING REMAINING ALLOCATED BLOCKS\n");

   /* Free all remaining allocated blocks */
   for (i = 0; i < NUM_BLOCKS; i++) {
      if (!isFree[i]) {
         free(res[i]);
         isFree[idx] = 0;
      }
   }

   brkEnd = sbrk(0);

   if ((char*)brkEnd - (char*)brkStart >=16) {
      print("FAILED: brk is not back to the beginning\n");
      dprint_ptr(0, "Starting brk val", brkStart);
      dprint_ptr(0, "End brk val", brkEnd);
   } else {
      print("PASSED: No segfaults :D\n");
   }
      
   dprint_int(0, "Diff", (int)((char*)brkEnd - (char*)brkStart));
}

