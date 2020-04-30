/* Ryan Gelston
 * Filename: print.c
 * Description: Contains functions for printing to the console when debugging 
 *    malloc.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "print.h"

/**********************************************************
 * Printing Functions
 **********************************************************/

/* Prints a string to the console */
int print(char* str) {

   int size = 0;
   int res;

   /* Fine the length of the string */
   while (str[size]) {
      size++;
   }

   res = write(STDERR_FILENO, (void*) str, size);

   if (res == -1) {
      /* In case it was a problem with the str ptr, we'll write a message */
      write(STDERR_FILENO, "Unable to print to the console\n", 31); 
      exit(-1); 
   }

   return res;
}

/* Prints a debug string */
int dprint(int level, char* str) {
  
   const char* debug = getenv("DEBUG_MALLOC");
   int debugLevel = 0; /* Default debug level is 0 */
   if (debug != NULL) {
      debugLevel = atoi(debug); 
   }
   if (level <= debugLevel) {
      return print(str);
   }
   
   return 0;
}

/* Prints a string for a malloc call */
int malloc_print(size_t size, void* res, size_t adjSize) {

   char msg[250];
   sprintf((char*)&msg, "MALLOC: malloc(%d)   =>   (ptr=%p, size=%d)\n",
           (int)size, res, (int)adjSize);
   return dprint(1, msg);
}


/* Prints a string for a calloc call */
int calloc_print(size_t num, size_t size, void* res, size_t adjSize) {

   char msg[250];
   sprintf((char*)&msg, "MALLOC: calloc(%d,%d)   =>   (ptr=%p, size=%d)\n",
           (int)num, (int)size, res, (int)adjSize);
   return dprint(1, msg);
}


/* Prints a string for a realloc call */
int realloc_print(void* ptr, size_t size, void* res, size_t adjSize) {

   char msg[250];
   sprintf((char*)&msg, "MALLOC: realloc(%p,%d)   =>   (ptr=%p, size=%d)\n",
           ptr, (int)size, res, (int)adjSize);
   return dprint(1, msg);
}


/* Prints a string for a free call */
int free_print(void* ptr) {

   char msg[250];
   sprintf((char*)&msg, "MALLOC: free(%p)\n", ptr);
   return dprint(1, msg);
}


/* Prints a pointer address after a given message */
int dprint_ptr(int level, char* str, void *ptr) {

   char msg[250];
   sprintf((char*)&msg, "%s: %p\n", str, ptr);
   return dprint(level, msg);
}


/* Prints a pointer address after a given message */
int dprint_int(int level, char* str, size_t num) {

   char msg[250];
   sprintf((char*)&msg, "%s: %d\n", str, (int)num);
   return dprint(level, msg);
}
