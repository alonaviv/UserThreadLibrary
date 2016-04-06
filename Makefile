CC = g++
LIB_OBJECTS = thread_classes.cpp uthreads.cpp general_macros.h
FLAGS = -std=c++11 -Wall

main: ${LIB_OBJECTS}
	${CC} ${FLAGS} -c thread_classes.cpp -o thread_classes.o
	${CC} ${FLAGS} -c uthreads.cpp -o uthreads.o
	ar rcs libuthreads.a thread_classes.o uthreads.o
	
tar:
	tar cfv ex2.tar README Makefile thread_classes.h ${LIB_OBJECTS}
	
clean:
	rm libuthreads.a thread_classes.o uthreads.o ex2.tar

