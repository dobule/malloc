/* Ryan Gelston
 * Filename: print.h
 * Description: Header file for print.c, which contains printing functions
 *    which don't use malloc.
 */

#ifndef PRINT_H
#define PRINT_H

int print(char* str);
int dprint(int level, char* str);
int malloc_print(size_t size, void* res, size_t adjSize);
int calloc_print(size_t num, size_t size, void* res, size_t adjSize);
int realloc_print(void* ptr, size_t size, void* res, size_t adjSize);
int free_print(void* ptr);
int dprint_ptr(int level, char* str, void* ptr);
int dprint_int(int level, char* str, size_t num);

#endif
