/*This module holds the declarations of the classes (and their helper 
functions) used by the uthreads library */

#ifndef _THREADS_CLASSES_
#define _THREADS_CLASSES_

#include <unordered_map>
#include <list>
#include <bitset>

#define NOT_SLEEPING -1

#include <sys/time.h>

#include <setjmp.h>
#include <signal.h>


#include "uthreads.h"
#include "general_macros.h" 


enum State{SLEEPING,READY,RUNNING,BLOCKED};


/* helper functions to allow threads to allocate and free stacks */ 
char* getNewStack();
void deleteStack(char* stackPtr);


/* This class holds information about a certain thread - 
its id, state, time until it wakes up, and actual running time so far.
*/
class Thread
{
public:
	Thread(int id);
	Thread(int id, void (*f)(void));
	~Thread(){deleteStack(_SP);}
	int getId(){ return _id; }
	int getQuantumsTillWakeup(){ return _quantumsTillWakeup; }
	int getQuantumRuntime(){ return _quantumRuntime; }
	enum State getState(){ return _state; }
	void decrementQuantumsTillWakeup();
	void incrementQuantumRuntime();
	int setQuantumsTillWakeup(int quantumsTillWakeup);
	void setState(State state){_state = state;}
	sigjmp_buf* getEnv(){return &_env;}
		
	
private:
	int _id;
	int _quantumsTillWakeup; 
	int _quantumRuntime;
	enum State _state;
	sigjmp_buf _env;
	char* _SP;
	
};

/* This class wraps a collection which holds all thread classes 
that are in play. Enables retriving the reference to the thread of
a given id, deleting the pointer of a thread with a given id, and adding a 
thread to the collection. Implemented with a hash table. 
The class perfoms sanity checks on the operations, to make sure the requested
id exists, and that the number of threads does not exceed
uthreads::MAX_THREAD_NUM*/

class ThreadCollection
{
public:
	void add(Thread *thread);
	void remove(int threadId);
	Thread* get(int threadId);
	int size(){return _size;}
	void deleteAllThreads();
	
private:
	std::unordered_map<int,Thread*> _threadMap;
	int _size;
	
};

/* This class wraps an itimerval timer, and supplies an interface for setting
and resetting the timer with given usecs. */

class Timer
{
public:
	Timer(int usecs):_usecs(usecs){setTimer(_usecs);}
	void reset(){setTimer(_usecs);}
private:
	void setTimer(int usecs);
	int _usecs;
	struct itimerval _itimerval;
	
};

/* This class wraps a list which holds pointers to all threads currently 
ready to be executed. Supplies an interface to add, pop, and remove from 
middle of queue*/

class ReadyQueue
{
public:
	void add(Thread* thread);
	Thread* pop();
	void remove(int id);
	void remove(Thread* thread);
	bool notEmpty();
	
private:
	std::list<Thread*> _list;
	
};

/* This class wraps a list which holds pointer to all sleeping threads.
The class supplies an interface to add threads to the list, decrement
the sleep counter, wake up threads who's time is up, and delete threads
from the list */
class SleepManager
{
public:
	void add(Thread* thread);
	void decrementThreads();
	void wakeUpSleepers(ReadyQueue* readyQueuePtr);
	void remove(Thread* thread);


private:
	std::list<Thread*> _list;
};

/* This class distributes id numbers for new threads, giving them the smallest
id not already taken by an existing thread. Once an id is distrbuted, the 
class assumes it is being used, until told otherwise. Internally implemented
using a bitset. */

// Bits valued 1 denote distributed ids, and valued 0 denote ids not yet taken

class IdDistributor
{
// Bits valued 1 denote distributed ids, and valued 0 denote ids not yet taken

public:
	int distribute();
	void freeId(int id);
	
private:
	std::bitset<MAX_THREAD_NUM> _bitset;	
};


#endif


