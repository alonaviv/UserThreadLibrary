/* implemenation of the thread_classes header */

#include "thread_classes.h"
#include <assert.h>

#define NDEBUG

using namespace std;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif


/*Thread constructor for main thread. Throws exception if stack can't be 
allocated*/

Thread::Thread(int id)
{
	_id = id;
	_quantumsTillWakeup = NOT_SLEEPING;
	_quantumRuntime = 0;
	_state = READY;
	try
	{
		_SP = getNewStack();	
	}
	catch(const char* e)
	{
		throw e;		
	}

}

/*Thread constructor for new threads. Throws exception if stack can't be 
allocated*/

Thread::Thread(int id, void (*f)(void))
{
	_id = id;
	_quantumsTillWakeup = NOT_SLEEPING;
	_quantumRuntime = 0;
	_state = READY;
	
	try
	{
		_SP = getNewStack();	
	}
	catch(const char* e)
	{
		throw e;		
	}

	//setting up first thread environment
	address_t sp = (address_t)_SP + STACK_SIZE - sizeof(address_t);
	address_t pc = (address_t)f;
	
	sigsetjmp(_env,1);
	(_env->__jmpbuf)[JB_SP] = translate_address(sp);
	(_env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&_env->__saved_mask); 
	
}


/* Decrements _quantumsTillWakeup by one */
void Thread::decrementQuantumsTillWakeup()
{
	_quantumsTillWakeup --;
	assert(_quantumsTillWakeup >= 0);
}


/* increments _quantumRuntime by one. */
void Thread::incrementQuantumRuntime()
{
	_quantumRuntime ++;
}


/*Sets the amount of quantums until wakup. Number must be positive.
returns 0 on success, -1 on failure */
int Thread::setQuantumsTillWakeup(int quantumsTillWakeup)
{
	if(quantumsTillWakeup < 0)
	{
		return FUNCTION_FAIL;
	}
	
	_quantumsTillWakeup = quantumsTillWakeup;
	return FUNCTION_SUCCESS;
}



/* Receives a thread to add to the collection */
void ThreadCollection::add(Thread *thread)
{
	assert(thread != nullptr && thread !=NULL);
			
	std::pair<int,Thread*> newItem (thread -> getId(),thread);
	
	assert(std::get<1>(_threadMap.insert(newItem)));
	_size++;
}


/* Deletes the thread  pointer with given id, if the thread exists. 
Throws an exception on failure. */
void ThreadCollection::remove(int threadId)
{
	_threadMap.erase(threadId);
	_size--;
}


/* Retrives the thread with given id, if the thread exists. 
Throws an exception on failure */
Thread* ThreadCollection::get(int threadId)
{
	return _threadMap.at(threadId); 
}


/*Deletes all thread objects who's pointers are stored in the collection */
void ThreadCollection::deleteAllThreads()
{
	for(auto iter = _threadMap.begin(); iter != _threadMap.end(); ++iter)
	{
		delete iter -> second;
	}
	
}


/* Sets and starts a ITIMER_VIRTUAL timer according to the given usecs.
 In case of an error in the system call, an error is printed and the 
 entire process is exited */
void Timer::setTimer(int usecs)
{
	_itimerval.it_value.tv_sec = (int)usecs/1000000;
	_itimerval.it_value.tv_usec = usecs%1000000;
	_itimerval.it_interval.tv_sec = (int)usecs/1000000;
	_itimerval.it_interval.tv_usec = usecs%1000000;
	
	if(setitimer(ITIMER_VIRTUAL, &_itimerval, NULL))
	{
		fprintf(stderr, "system error: Can't set timer\n");
		exit(1);
	}

}

/*Adds a thread pointer to queue*/
void ReadyQueue::add(Thread* thread)
{
	assert(thread != nullptr && thread !=NULL);
	_list.push_front(thread);
}



/* Pops and returns thread that was inserted first. Expects queue to be
non empty. */
Thread* ReadyQueue::pop()
{

	assert(!_list.empty());
	Thread* thread = _list.back();
	_list.pop_back();

	return thread;
	
}


/* Removes thread with given id number from queue. If thread doesn't exist,
does nothing. */
void ReadyQueue::remove(int id)
{
	std::list<Thread*>::iterator iter = _list.begin();
	while(iter != _list.end())
	{
		if((*iter) -> getId() == id)
		{
			_list.erase(iter);
			return;
		}
		iter++;
		
	}
}

/* Removes thread pointer from queue. If thread doesn't exist,
does nothing. */
void ReadyQueue::remove(Thread* thread)
{
	std::list<Thread*>::iterator iter = _list.begin();
	while(iter != _list.end())
	{
		if((*iter) == thread)
		{
			_list.erase(iter);
			return;
		}
		iter++;
		
	}

}

/* Returns true if queue is non empty, and false otherwise. */
bool ReadyQueue::notEmpty(){
	return !_list.empty();	
}

/* Adds a thread pointer to the sleepers list */
void SleepManager::add(Thread* thread)
{
	assert(thread != nullptr && thread !=NULL);
	_list.push_front(thread);
}

/*Decrements the sleep timer for all threads in the list.
threads who reached the end of thier sleep time are removed from
list at least every quantum, so there can be no negative timer values */
void SleepManager::decrementThreads()
{

	std::list<Thread*>::iterator iter = _list.begin();
	while(iter != _list.end())
	{
		(*iter) -> decrementQuantumsTillWakeup();
		assert((*iter) -> getQuantumsTillWakeup() >=0);
		iter++;
	}
	
}

/*Iterates the sleepers list, and checks which threads have 0 quantums left
untill wakeup. Wakes thoes threads up by changing thier state, removing them
from the sleepers list and adding them to the ready list */
void SleepManager::wakeUpSleepers(ReadyQueue* readyQueuePtr)
{
	std::list<Thread*>::iterator iter = _list.begin();
	while(iter != _list.end())
	{
		if((*iter) -> getQuantumsTillWakeup() ==0)
		{
			(*iter) -> setState(READY);
			readyQueuePtr -> add(*iter);
			iter = _list.erase(iter);
			continue;
		}
		
		iter++;
	}

}


/* Removes thread pointer from sleepers list. If thread doesn't exist,
does nothing. */
void SleepManager::remove(Thread* thread)
{
	std::list<Thread*>::iterator iter = _list.begin();
	while(iter != _list.end())
	{
		if((*iter) == thread)
		{
			_list.erase(iter);
			return;
		}
		iter++;
		
	}
}


/* Distrubutes the lowest non-taken id */
int IdDistributor::distribute()
{
// Bits valued 1 denote distributed ids, and valued 0 denote ids not yet taken

	for(int i=0; i< (int)_bitset.size(); i++)
	{
		if(_bitset[i]==0)
		{
			_bitset[i] = 1; //reserving chosen id
			return i;
		}
	}
	assert(0); //If no id can be ditributed - bug in program(too many threads
	          //created)
}

/*frees the given id, so it can be redistributed to a new thread */ 
void IdDistributor::freeId(int id)
{
// Bits valued 1 denote distributed ids, and valued 0 denote ids not yet taken
	assert(id < (int) _bitset.size());
	_bitset[id] = 0;	
}



/* Allocates and returns a new stack with fixed size STACK_SIZE */
char* getNewStack()
{
	char* newStack = (char*)malloc(STACK_SIZE*sizeof(char));
	if(newStack == NULL)
	{
		fprintf(stderr, "system error: Can't allocate new thread's stack "\
		"memory\n");
		throw "can't allocate stack";
	}
	
	return newStack;	
	
}

/*  Frees the memory of given stack */
void deleteStack(char* stackPtr)
{
	assert(stackPtr != nullptr && stackPtr !=NULL);
	free(stackPtr);
	
}



