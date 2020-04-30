# Name: Ryan Gelston
# Class: CSC-435
# Assingnment: Asgn1


CC=gcc
FLAGS=-Wall -pedantic -g
CFLAGS=-Wall -pedantic -g -fpic


clean: 
	rm *.o *.gch test_malloc

clear:
	clear

print32.o: print.c print.h
	$(CC) $(CFLAGS) -m32 -c -o $@ print.c

print64.o: print.c print.h
	$(CC) $(CFLAGS) -m64 -c -o $@ print.c

intel-all: lib/libmalloc.so lib64/libmalloc.so                                  
	
lib/libmalloc.so: lib malloc32.o print32.o                                   
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o print32.o

lib64/libmalloc.so: lib64 malloc64.o print64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o print64.o                       

lib:                                                                            
	mkdir lib                                                                    

lib64:                                                                          
	mkdir lib64                                                                  

malloc32.o: malloc.c print32.o 
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c       

malloc64.o: malloc.c print64.o                                                   
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

setVars:                                                                        
	LD_LIBRARY_PATH=~/Documents/csc453/Asgn1/lib64:$$LD_LIBRARY_PATH
	export LD_LIBRARY_PATH  

test: test_malloc.c malloc64.o
	$(CC) $(FLAGS) -o test test_malloc.c malloc64.o print64.o
