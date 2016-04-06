/* Implmentation of the uthreads library. This library provides support for
user threads. Threads created using the library will be scheduled using a 
"round robin" algorithm. The function uthread_init MUST be invoked before 
using any other service */

#include "uthreads.h"
#include <signal.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>

#include "thread_classes.h"
#include "general_macros.h" 

#define NEDBUG

#define MAIN_ID 0
#define JMP_VALUE 1

using namespace std;

Thread* runningThread = nullptr;
ThreadCollection* collection = nullptr;
Timer* timer = nullptr;
ReadyQueue* readyQueue = nullptr;
SleepManager* sleepManager = nullptr;
IdDistributor* idDistributor = nullptr;

sigset_t alarmSignalSet;
int totalQuantumCounter = 0;

void scheduler(bool calledByQuantumManager);
void switchThreads(Thread* runnerUp);
void quantumHandler(int sigNum);
void installSIGVTALRMHandler();
void maskSIGVRALRM();
void unmaskSIGVRALRM();
void ignorePendingSIGVTALRM();
void cleanAndAbort(int exitSig);


/* This function removes the next thread in the queue and activates it. 
Additionally, it runs the sleeperManager's function which wakes up sleeping 
threads. Also, If scheduler was called from the quantumManager (notified by
parameter) moves runnin thread into the ready list. */

void scheduler(bool calledByQuantumManager=false)
{
	
	totalQuantumCounter++;
	
	
	//Dealing with sleepers
	sleepManager -> wakeUpSleepers(readyQueue);
	
	sleepManager -> decrementThreads();
	
	//If quantum manager called the scheduler, preempting the running thread
	// and moving it to the ready list
	if(calledByQuantumManager)
	{
		runningThread -> setState(READY);
		readyQueue -> add(runningThread);	
	}
	
	
	// Popping out next thread and running it
	Thread* nextThread  = readyQueue -> pop();
	assert(nextThread -> getState() == READY);
	
	nextThread -> setState(RUNNING);
	nextThread -> incrementQuantumRuntime();
	
	switchThreads(nextThread);
}


/* saves the environment of the current running thread, and runs the given
thread. When a thread is resumed, it returns to action from this point.
Before loading the new thread, the timer is reset, so it receives a single 
quantum at most to run */

void switchThreads(Thread* runnerUp)
{
	assert(runnerUp -> getState() == RUNNING);
	
	ignorePendingSIGVTALRM(); // Ignoring pending alarm signals that might 
							  // have occured during the context switch, 
							  // allowing the next thread a full quantum
							  
	int retVal = sigsetjmp(*(runningThread -> getEnv()),1);
	if(retVal == JMP_VALUE)
	{
		return;
	}

	runningThread = runnerUp;
	timer -> reset();

	siglongjmp(*(runnerUp -> getEnv()),JMP_VALUE);
	
}





/* Handles the operation each time a quantum is up. It preempts the 
currently running thread and moves it the ready list, and alls the scheduler
in order to let the next thread run*/

void quantumHandler(int sigNum)
{
	assert(runningThread -> getState() == RUNNING);
	
	//notifying scheduler that the quantum handler made the call
	scheduler(true);
}


/*Installs the funtion quantumHandler as the handler for signal SIGVTALRM */
void installSIGVTALRMHandler()
{
	struct sigaction signal;
	
	signal.sa_handler = &quantumHandler;
	
	if(sigaction(SIGVTALRM, &signal, NULL) == FUNCTION_FAIL)
	{
		fprintf(stderr, "system error: Can't install signal handler\n");
		cleanAndAbort(1);
	}
}

/*Masks the SIGVTALRM signal*/
void maskSIGVRALRM()
{
	int retVal = sigprocmask(SIG_SETMASK, &alarmSignalSet, NULL);
	if(retVal == FUNCTION_FAIL)
	{
		fprintf(stderr, "system error: Can't mask signal SIGVTALRM\n");
		cleanAndAbort(1);		
	}
	
}

/*Removes the mask from SIGVTALRM */
void unmaskSIGVRALRM()
{
	int retVal = sigprocmask(SIG_UNBLOCK, &alarmSignalSet, NULL);
	if(retVal == FUNCTION_FAIL)
	{
		fprintf(stderr, "system error: Can't unmask signal SIGVTALRM\n");
		cleanAndAbort(1);		
	}
}

/*Checks if SIGVTALRM is pending. If so, ignores the pending signal, so the
assigned signal handler isn't called */

void ignorePendingSIGVTALRM()
{
	sigset_t pendingSignals;
	
	int retVal = sigpending(&pendingSignals);
	if(retVal == FUNCTION_FAIL)
	{
		fprintf(stderr, "system error: Error on system call 'sigpending'\n");
		cleanAndAbort(1);		
	}
	
	retVal = sigismember(&pendingSignals, SIGVTALRM);
	if(retVal == FUNCTION_FAIL)
	{
		fprintf(stderr, "system error: Error on system call 'sigismember'\n");
		cleanAndAbort(1);		
	}
	
	//If SIGVTALRM is pending, ignore the signal
	if(retVal == 1)
	{
		struct sigaction ignoreAction;
		ignoreAction.sa_handler = SIG_IGN;
		
		if(sigaction(SIGVTALRM, &ignoreAction, NULL) == FUNCTION_FAIL)
		{
			fprintf(stderr, "system error: can't install signal handler\n");
			cleanAndAbort(1);
		}
		
		installSIGVTALRMHandler();	
	}
	
}


/* Frees all resources of program and aborts with given exit signal */
void cleanAndAbort(int exitSig)
{
	delete readyQueue;
	delete sleepManager;
	collection -> deleteAllThreads();
	delete collection;
	delete timer;
	delete idDistributor;
	
	exit(exitSig);
}


/*
 * Description: This function initializes the thread library. 
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantumUsecs)
{
	maskSIGVRALRM();
	if(quantumUsecs <= 0)
	{
		unmaskSIGVRALRM();
		fprintf(stderr,"thread library error: quantum usecs must be"\
		" positive\n");
		return FUNCTION_FAIL;
	}
	
	installSIGVTALRMHandler();
	
	// Note -  creating timer encompases a system calls that might fail. 
	// In case of failure the program will exit from within the timer
	//constructor. No need to release resources, as nothing has been 
	//allocated yet. 
	
	
	//Creating neccesary objects.

	timer = new Timer(quantumUsecs);
	// Note -  creating timer encompases a system calls that might fail. 
	// In case of failure the program will exit from within the timer
	//constructor. No need to release resources, as nothing has been 
	//allocated yet. 
	
	collection = new ThreadCollection();
	readyQueue = new ReadyQueue();
	sleepManager = new SleepManager();
	idDistributor = new IdDistributor();
	
	//setting the set to hold only SIGVTALRM)
	sigemptyset(&alarmSignalSet);	
	sigaddset(&alarmSignalSet,SIGVTALRM);	
	
	//Creating main thread
	Thread* mainThread;
	// If memory for stack can't be allocated, abort program with exit code 1.
	try
	{
		mainThread = new Thread(idDistributor -> distribute()); 	
	}
	catch(const char* e)
	{
		cleanAndAbort(1);		
	}
	
	collection -> add(mainThread);
	readyQueue -> add(mainThread);
	
	runningThread = mainThread;
	scheduler();
	
	unmaskSIGVRALRM();
	return FUNCTION_SUCCESS; 
}


/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void))
{
	
	maskSIGVRALRM();

	if(collection -> size() >= MAX_THREAD_NUM)
	{
		unmaskSIGVRALRM();
		fprintf(stderr,"thread library error: you reached the max number "\
		"of threads\n");
		return FUNCTION_FAIL;
	}
	
	
	Thread* newThread;
	// If memory for stack can't be allocated, abort program with exit code 1.
	try
	{
		newThread = new Thread(idDistributor -> distribute(),f);
	}
	catch(const char* e)
	{
		cleanAndAbort(1);		
	}
	
	
	collection -> add(newThread);
	readyQueue -> add(newThread);

	
	
	unmaskSIGVRALRM();
	return newThread -> getId();
}


/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered as an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory]. 
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid)
{

	maskSIGVRALRM();

	Thread* thread;
	int runningThreadId = runningThread -> getId();
		
	try
	{
		thread = collection -> get(tid);
	}
	catch(const std::out_of_range)
	{
		fprintf(stderr, "thread library error: Trying to terminate "\
		"non-existant thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	
	//It ias an error to try and kill a sleeping thread
	if(thread -> getState() == SLEEPING)
	{
		fprintf(stderr, "thread library error: Trying to terminate "\
		"a sleeping thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
		
	//If the given thread was the main thread, scheduler is run in order to
	//replace it (the thread will no longer run as the object and all pointers
	//have been deleted)
	if(tid == MAIN_ID)
	{
		cleanAndAbort(0);
	}
	
	//Delete given thread

	collection -> remove(tid); //n ote - this function throws an exception,
							   //but this would have been caught by the catch
							   //above
							   
	readyQueue -> remove(thread);
	sleepManager -> remove(thread);
	idDistributor -> freeId(tid);
	delete thread;	
	
	//If the main thread is being deleted, delete all threads, remove all
	//resources and exit process
	
	if(tid ==runningThreadId)
	{
		scheduler();
	}
	
	unmaskSIGVRALRM();
	return FUNCTION_SUCCESS;		
}


/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED or SLEEPING states has no
 * effect and is not considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
	maskSIGVRALRM();
	
	if(tid == MAIN_ID)
	{
		fprintf(stderr, "thread library error: Trying to block main "\
		"thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	Thread* thread;
	
	try
	{
		thread = collection -> get(tid);
	}
	catch(const std::out_of_range)
	{
		fprintf(stderr, "thread library error: Trying to block non-"\
		"existant thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	
	switch(thread -> getState()){
		//In case thread is currently running, the next runner up will 
		//be inserted instead.
		case RUNNING: 
			assert(runningThread -> getId() == tid);
			thread -> setState(BLOCKED);
			scheduler();
			break;
			
		//If thread was in the waiting queue, it is removed.
		case READY:
			readyQueue -> remove(tid);
			thread -> setState(BLOCKED);
			break;
		default:
			//No action for other states
			break;
	}	
	
	unmaskSIGVRALRM();
	return FUNCTION_SUCCESS;
}


/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in the RUNNING, READY or SLEEPING
 * state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered as an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid)
{

	maskSIGVRALRM();
	
	Thread* thread;
	
	try
	{
		thread = collection -> get(tid);
	}
	catch(const std::out_of_range)
	{
		fprintf(stderr, "thread library error: Trying to resume non-" \
		"existant thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	
	if(thread -> getState() == BLOCKED)
	{
		thread -> setState(READY);
		readyQueue -> add(thread);
	}

	unmaskSIGVRALRM();
	return FUNCTION_SUCCESS;
}

/*
 * Description: This function puts the RUNNING thread to sleep for a period
 * of num_quantums (not including the current quantum) after which it is moved
 * to the READY state. num_quantums must be a positive number. It is an error
 * to try to put the main thread (tid==0) to sleep. Immediately after a thread
 * transitions to the SLEEPING state a scheduling decision should be made.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums)
{
	maskSIGVRALRM();
	
	if(runningThread -> getId() == MAIN_ID)
	{
		fprintf(stderr, "thread library error: Trying to put main "\
		"thread to sleep\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;		
	}
	
	if(num_quantums <= 0)
	{
		fprintf(stderr, "thread library error: num_quantums for sleep call"\
		" must be positive\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;		
	}
	
	
	assert(runningThread -> getState() == RUNNING);
	runningThread -> setState(SLEEPING);
	runningThread -> setQuantumsTillWakeup(num_quantums);
	sleepManager -> add(runningThread);
	scheduler();
	
	unmaskSIGVRALRM();
	return FUNCTION_SUCCESS;	
	
}

/*
 * Description: This function returns the number of quantums until the thread
 * with id tid wakes up including the current quantum. If no thread with ID
 * tid exists it is considered as an error. If the thread is not sleeping,
 * the function should return 0.
 * Return value: Number of quantums (including current quantum) until wakeup.
*/
int uthread_get_time_until_wakeup(int tid)
{
	maskSIGVRALRM();
	Thread* thread;
		
	try
	{
		thread = collection -> get(tid);
	}
	catch(const std::out_of_range)
	{
		fprintf(stderr, "thread library error: Trying to get time until "\
		"wakeup for non-existant thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	
	if(thread -> getState() == SLEEPING)
	{
		//Since implementation decrements this value at the
		//start of each quantum, 1 must be added in order to obtain
		//true value
		unmaskSIGVRALRM();
		return thread -> getQuantumsTillWakeup() + 1;
	}
	else{
		unmaskSIGVRALRM();
		return 0;
	}
	
}


/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/

int uthread_get_tid()
{
	
	maskSIGVRALRM();
	int id = runningThread -> getId();
	
	unmaskSIGVRALRM();
	return id;
}

/*
 * Description: This function returns the total number of quantums that were
 * started since the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums()
{
	return totalQuantumCounter;	
}



/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered as an error.
 * Return value: On success, return the number of quantums of the 
 * thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid)
{
	maskSIGVRALRM();
	Thread* thread;
		
	try
	{
		thread = collection -> get(tid);
	}
	catch(const std::out_of_range)
	{
		fprintf(stderr, "thread library error: Trying to get quantums for "\
		"non-existant thread\n");
		unmaskSIGVRALRM();
		return FUNCTION_FAIL;
	}
	
	unmaskSIGVRALRM();
	return thread -> getQuantumRuntime();
	
}
