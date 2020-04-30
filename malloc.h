/* Name: Ryan Gelston
 * Filename: malloc.h
 * Description: Header file for malloc.c
 */

#ifndef MALLOC_H
#define MALLOC_H

void* malloc(size_t size);

void* calloc(size_t num, size_t size);

void* realloc(void* ptr, size_t new_size);

void free(void* ptr);

#endif
