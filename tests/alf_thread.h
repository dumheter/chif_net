// MIT License
//
// Copyright (c) 2018-2019 Filip Björklund
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ALF_THREAD_H
#define ALF_THREAD_H

#if defined(__cplusplus)
extern "C" {
#endif

// ========================================================================== //
// Header includes
// ========================================================================== //

// Standard headers
#include <stdint.h>

// ========================================================================== //
// Type definitions
// ========================================================================== //

/** Boolean type **/
typedef uint32_t AlfBool;

// -------------------------------------------------------------------------- //

/** Prototype for a function that can be used as the starting-point of a newly 
 * spawned thread. 
 * \brief Thread start function.
 * \param argument Argument to the function.
 */
typedef uint32_t(*PFN_AlfThreadFunction)(void* argument);

// -------------------------------------------------------------------------- //

/** Predicate function that is used for condition variables.
 * \brief Predicate function.
 * \param argument Argument to predicate function.
 */
typedef AlfBool(*PFN_AlfPredicate)(void* argument);

// ========================================================================== //
// Macro declarations
// ========================================================================== //

// True value
#ifndef ALF_TRUE
	/** Value for a boolean representing true **/
#	define ALF_TRUE ((AlfBool)1)
#endif

// False value
#ifndef ALF_FALSE
	/** Value for a boolean representing false **/
#	define ALF_FALSE ((AlfBool)0)
#endif

// Constant for immediate timeout
#define ALF_THREAD_IMMEDIATELY (0)

// Allocation macros
#if !defined(ALF_THREAD_ALLOC)
#	include <stdlib.h>
	/** Allocation function **/
#	define ALF_THREAD_ALLOC(size) malloc(size)
#endif

// Allocation macros
#if !defined(ALF_THREAD_FREE)
#	include <stdlib.h>
	/** Free the memory of an allocation **/
#	define ALF_THREAD_FREE(mem) free(mem)
#endif

// Assert macros
#if !defined(ALF_THREAD_ASSERT)
#	include <assert.h>
	/** Internal assertion macro **/
#	define ALF_THREAD_ASSERT(cond, msg) assert((cond) && msg)
#endif 

// Default thread name
#define ALF_DEFAULT_THREAD_NAME "Unknown"

// ========================================================================== //
// Enumerations
// ========================================================================== //

/** \enum AlfThreadPriority
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:23
 * \brief Thread priorities.
 * \details
 * Enumeration of thread priorities.
 * \see alfSetThreadPriority
 */
typedef enum AlfThreadPriority
{
	/** Lowest thread priority **/
	ALF_THREAD_PRIORITY_LOWEST,
	/** Low thread priority **/
	ALF_THREAD_PRIORITY_LOW,
	/** Normal thread priority **/
	ALF_THREAD_PRIORITY_NORMAL,
	/** High thread priority **/
	ALF_THREAD_PRIORITY_HIGH,
	/** Highest thread priority **/
	ALF_THREAD_PRIORITY_HIGHEST,
	/** Critical thread priority **/
	ALF_THREAD_PRIORITY_CRITICAL
} AlfThreadPriority;

// -------------------------------------------------------------------------- //

/** \enum AlfCache
 * \author Filip Björklund
 * \date 08 januari 2019 - 20:32
 * \brief Caches in a CPU.
 * \details
 * Enumeration of caches that may be available in a CPU.
 */
typedef enum AlfCache
{
	/** Level 1 data cache **/
	ALF_CACHE_L1D,
	/** Level 1 instruction cache **/
	ALF_CACHE_L1I,
	/** Level 2 combined cache **/
	ALF_CACHE_L2,
	/** Level 3 combined cache **/
	ALF_CACHE_L3,
	/** Level 4 combined cache **/
	ALF_CACHE_L4,
} AlfCache;

// ========================================================================== //
// Structures
// ========================================================================== //

/** \struct AlfThread
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:11
 * \brief Thread handle.
 * \details
 * Represents a handle to a single thread of execution on the system.
 */
typedef struct tag_AlfThread AlfThread;

// -------------------------------------------------------------------------- //

/** \struct AlfSemaphore
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:25
 * \brief Semaphore.
 * \details
 * Structure that represents a semaphore that is used to ensure that only a set
 * of threads may enter a section at one time.
 */
typedef struct tag_AlfSemaphore AlfSemaphore;

// -------------------------------------------------------------------------- //

/** \struct AlfMutex
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:25
 * \brief Mutex.
 * \details
 * Structure that represents a mutex, mutual exclusion, that is used to ensure 
 * that only one thread is executing a section of code at one time.
 */
typedef struct tag_AlfMutex AlfMutex;

// -------------------------------------------------------------------------- //

/** \struct AlfConditionVariable
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:25
 * \brief Condition variable.
 * \details
 * Structure that represents a condition variable for threads to wait on until a
 * condition is true and they are notified.
 */
typedef struct tag_AlfConditionVariable AlfConditionVariable;

// -------------------------------------------------------------------------- //

/** \struct AlfReadWriteLock
 * \author Filip Björklund
 * \date 08 januari 2019 - 21:47
 * \brief Read-write lock.
 * \details
 * Structure that represents a locking primitive that can be locked in both read
 * and write mode. Multiple readers can hold the lock at the same time, however
 * only one writer can hold it simultaneously.
 */
typedef struct tag_AlfReadWriteLock AlfReadWriteLock;

// -------------------------------------------------------------------------- //

/** \struct AlfTLSHandle
 * \author Filip Björklund
 * \date 03 oktober 2018 - 16:26
 * \brief Thread-local storage handle.
 * \details
 * Handle to thread-local storage. This is used as a key to store and retrieve
 * data that is local to the calling thread.
 */
typedef struct tag_AlfTLSHandle AlfTLSHandle;

// ========================================================================== //
// Functions
// ========================================================================== //

/** Function that must be called before the AlfThread library is used. This sets
 * up all the required data that is used by the library to keep track of thread
 * handles.
 * \brief Startup AlfThread.
 */
void alfThreadStartup(void);

// -------------------------------------------------------------------------- //

/** Function that can be called after the user is done using the AlfThread 
 * library. This will free up any data used by the library and also delete any 
 * handles to threads that might still be held onto by the library.
 * \brief Shutdown AlfThread.
 */
void alfThreadShutdown(void);

// ========================================================================== //
// Thread functions
// ========================================================================== //

/** Create a thread that executes the specified function with the specified 
 * arguments. The name of the thread is set to a the default name ("Unknown").
 * \note Handles to threads are not freed by the user but rather when the thread
 * is either joined or detached. Any dangling thread handles will also be freed
 * when the shutdown function is called.
 * \brief Create thread.
 * \param function Function to run on thread.
 * \param argument Argument passed to function.
 * \return Handle to thread.
 */
AlfThread* alfCreateThread(PFN_AlfThreadFunction function, void* argument);

// -------------------------------------------------------------------------- //

/** Create a named thread that executes the specified function with the 
 * specified arguments. The name must be valid UTF-8.
 * \note Handles to threads are not freed by the user but rather when the thread
 * is either joined or detached. Any dangling thread handles will also be freed
 * when the shutdown function is called.
 * \note As there is a limitation on thread name lengths in pthread, only the
 * first 15 characters will be displayed in a debugger. However the full name is
 * still retrieved from a call to AlfGetThreadName.
 * \brief Create thread.
 * \param function Function to run on thread.
 * \param argument Argument passed to thread function.
 * \param name Name of the thread.
 * \return Handle to thread.
 */
AlfThread* alfCreateThreadNamed(
	PFN_AlfThreadFunction function, 
	void* argument, 
	const char* name);

// -------------------------------------------------------------------------- //

/** Returns the handle to the calling thread.
 * \brief Returns handle of calling thread.
 * \return Handle for the calling thread.
 */
AlfThread* alfThisThread(void);

// -------------------------------------------------------------------------- //

/** Wait for a thread. This will block until the thread represented by the 
 * handle is has finished executing its thread function.
 * \note Joining the same thread multiple times will result in undefined 
 * behaviour.
 * \brief Wait for thread.
 * \param thread Thread to wait for.
 * \return Exit code returned from the thread function.
 * \note Using a handle to a thread that has been joined is undefined behaviour 
 * and may also possibly end in a crash.
 */
uint32_t alfJoinThread(AlfThread* thread);

// -------------------------------------------------------------------------- //

/** Try to join a thread. If the thread has not yet exited then the function 
 * returns false. Else the thread will be joined and the function returns true.
 * \note Joining the same thread multiple times results in undefined behaviour.
 * \note Using a handle to a thread that has been joined is undefined behaviour
 * and may also possibly end in a crash.
 * \brief Wait for thread.
 * \param thread Thread to try to join.
 * \param exitCodeOut exit code from thread function, not set if function 
 * returns false.
 * \return True if the thread could be joined else false.
 */
AlfBool alfJoinThreadTry(AlfThread* thread, uint32_t* exitCodeOut);

// -------------------------------------------------------------------------- //

/** Detach a thread. This will let the thread continue running without having to
 * be joined at a later time. All resources used by the thread are releases when
 * the thread exits.
 * \note Using a handle to a thread that has been detached results in undefined
 * behaviour.
 * \note A thread that has been detached cannot be joined.
 * \brief Detach thread.
 * \param thread Handle to the thread that is to be detached.
 */
void alfDetachThread(AlfThread* thread);

// -------------------------------------------------------------------------- //

/** Kill a executing thread abruptly.
 * \brief Kill thread.
 * \param thread Thread to kill.
 */
void alfKillThread(AlfThread* thread);

// -------------------------------------------------------------------------- //

/** Stop the execution of the calling thread. The exit code returned from the 
 * thread is that of the argument to this function.
 * \brief Exit thread.
 * \param exitCode Exit code from thread.
 */
void alfExitThread(uint32_t exitCode);

// -------------------------------------------------------------------------- //

/** Yield the execution of the calling thread. This means that another thread
 * might be scheduled (if available).
 * \brief Yield calling thread.
 */
void alfYieldThread(void);

// -------------------------------------------------------------------------- //

/** Set the priority of a thread. This priority will affect how the operationg
 * system schedules the threads.
 * \note Apple systems does not support this function and it will therefore 
 * always return false.
 * \brief Set thread priority.
 * \param thread Thread to set priority of.
 * \param priority Prioriy to set.
 * \return True if the priority could be set othewise false.
 */
AlfBool alfSetThreadPriority(AlfThread* thread, AlfThreadPriority priority);

// -------------------------------------------------------------------------- //

/** Sleep the calling thread for the specified number of milliseconds. The
 * precision of the sleep depends on the platform and implementation.
 * \brief Sleep the calling thread for a specified number of milliseconds.
 * \param milliseconds Number of milliseconds to sleep thread for.
 */
void alfSleepThread(uint64_t milliseconds);

// -------------------------------------------------------------------------- //

/** Returns the id of the specified thread.
 * \brief Returns id of specified thread.
 * \param thread Thread to get ID of.
 * \return ID of the thread.
 */
uint32_t alfGetThreadID(AlfThread* thread);

// -------------------------------------------------------------------------- //

/** Returns the name of the calling thread. This name is owned by the thread and
 * must not be freed by the user.
 * \brief Returns name of thread.
 * \return Name of thread.
 */
const char* alfGetThreadName(void);

// -------------------------------------------------------------------------- //

/** Sets the name of the calling thread. The string will be copied and stored by
 * the thread internally.
 * \brief Set thread name.
 * \param name Name to set.
 * \return True if the name could be set, otherwise false.
 */
AlfBool alfSetThreadName(const char* name);

// ========================================================================== //
// Semaphore functions
// ========================================================================== //

/** Create a semaphore that is initialized with the specified value.
 * \brief Create semaphore.
 * \param initialValue Initial semaphore value.
 * \return Created semaphore.
 */
AlfSemaphore* alfCreateSemaphore(uint64_t initialValue);

// -------------------------------------------------------------------------- //

/** Delete a semaphore.
 * \note There must be no threads waiting on the semaphore when it's being 
 * deleted. Or else they will never be able to proceed.
 * \brief Delete semaphore
 * \param semaphore Semaphore to delete.
 */
void alfDeleteSemaphore(AlfSemaphore* semaphore);

// -------------------------------------------------------------------------- //

/** Acquire a semaphore. This will block until the semaphore value is at least
 * one, then it will be decremented and the function will return.
 * \brief Acquire Semaphore.
 * \param semaphore Semaphore to acquire.
 */
void alfAcquireSemaphore(AlfSemaphore* semaphore);

// -------------------------------------------------------------------------- //

/** Try to acquire a semaphore. This will block either until the semaphore is
 * released (i.e. value is greater than 0) or until the timeout expires.
 * \brief Acquire semaphore with timeout.
 * \param semaphore Semaphore to acquire.
 * \param milliseconds Milliseconds until timing out.
 * \return True if the semaphore was acquired otherwise false.
 */
AlfBool alfAcquireSemaphoreTimed(
	AlfSemaphore* semaphore, 
	uint64_t milliseconds);

// -------------------------------------------------------------------------- //

/** Checks if the semaphore can be acquired and acquires it if possible. If the
 * semaphore cannot immediately be acquired then the function returns false and
 * the semaphore is not acquired.
 * \note The result of calling this function is the same as calling 
 * alfAcquireSemaphoreTimed with a timeout of 0 (ALF_THREAD_IMMEDIATELY).
 * \brief Try to acquire semaphore.
 * \param semaphore Semaphore to try to acquire.
 * \return True if the semaphore was acquired else false.
 */
AlfBool alfAcquireSemaphoreTry(AlfSemaphore* semaphore);

// -------------------------------------------------------------------------- //
/** Release semaphore. This will increment the semaphore value by one and then
 * return.
 * \brief Release semaphore.
 * \param semaphore Semaphore to release.
 */
void alfReleaseSemaphore(AlfSemaphore* semaphore);

// ========================================================================== //
// Mutex functions
// ========================================================================== //

/** Create a mutex. The parameter is used to determine if the mutex should be 
 * created with support for recursive locking.
 * \brief Create mutex.
 * \param recursive Whether the mutex can be recursively locked.
 * \return Created mutex.
 */
AlfMutex* alfCreateMutex(AlfBool recursive);

// -------------------------------------------------------------------------- //

/** Delete a mutex. 
 * \note The user is responsible for making sure that the mutex is not held by
 * any thread when it is destroyd, as that results in undefined behaviour.
 * \brief Delete mutex.
 * \param mutex Mutex to destroy.
 */
void alfDeleteMutex(AlfMutex* mutex);

// -------------------------------------------------------------------------- //
/** Acquire a mutex. This function blocks until any other thread that holds the
 * mutex releases it.
 * \brief Acquire mutex.
 * \param mutex Mutex to acquire.
 */
void alfAcquireMutex(AlfMutex* mutex);

// -------------------------------------------------------------------------- //

/** Try to acquire a mutex. If the mutex is not locked by another thread it will
 * be locked and the function returns true. If the mutex on the other hand is 
 * locked by another thread then this function returns false.
 * \brief Try to acquire mutex.
 * \param mutex Mutex to acquire.
 * \return True if the mutex was acquired else false.
 */
AlfBool alfAcquireMutexTry(AlfMutex* mutex);

// -------------------------------------------------------------------------- //

/** Release mutex that is held by the calling thread. Calling this when the 
 * calling thread does not already hold the mutex will result in no operation.
 * \brief Release mutex.
 * \param mutex Mutex to release.
 */
void alfReleaseMutex(AlfMutex* mutex);

// -------------------------------------------------------------------------- //

/** Returns whether or not a mutex is recursive. A recursive mutex can be locked
 * multiple times from the same thread will still owned by the thread.
 * \brief Returns whether mutex is recursive.
 * \param mutex Mutex to check.
 * \return True if the mutex is recursive else false.
 */
AlfBool alfIsMutexRecursive(AlfMutex* mutex);

// ========================================================================== //
// Condition variable functions
// ========================================================================== //

/** Create a condition variable.
 * \brief Construct condition variable.
 * \return Created condition variable.
 */
AlfConditionVariable* alfCreateConditionVariable(void);

// -------------------------------------------------------------------------- //

/** Delete condition variable.
 * \note It's up to the user to make sure that there are no threads still 
 * waiting on the condition variable when it's destroyed.
 * \brief Delete condition variable.
 * \param conditionVariable Condition variable to destroy.
 */
void alfDeleteConditionVariable(AlfConditionVariable* conditionVariable);

// -------------------------------------------------------------------------- //

/** Wait for condition variable to be notified. This function may return early
 * due to so-called spurious waking. The predicate version of this function can
 * be used to handle this.
 * \brief Wait for condition variable.
 * \param conditionVariable Condition variable to wait for.
 * \param mutex Mutex that is unlocked during the wait.
 */
void alfWaitConditionVariable(
	AlfConditionVariable* conditionVariable, 
	AlfMutex* mutex);

// -------------------------------------------------------------------------- //

/** Wait for a condition variable to be notified and the predicate to hold. If
 * the condition variable is randomly notified without the condition holding
 * then the thread will go back to waiting on the condition. Condition variables
 * may be randomly awoken due to so-called spurious wakes, this function is used
 * to avoid this.
 * \brief Wait for condition variable and predicate.
 * \param conditionVariable Condition variable to wait for.
 * \param mutex Mutex to unlock during wait.
 * \param predicate Predicate that must hold when the condition is signaled.
 * \param predicateArgument Argument to the predicate function.
 */
void alfWaitConditionVariablePredicate(
	AlfConditionVariable* conditionVariable, 
	AlfMutex* mutex, 
	PFN_AlfPredicate predicate, 
	void* predicateArgument);

// -------------------------------------------------------------------------- //

/** Notify one thread that is waiting on condition variable.
 * \brief Notify one waiting thread on condition variable.
 * \param conditionVariable Condition variable to notify threads waiting on.
 */
void alfNotifyConditionVariable(AlfConditionVariable* conditionVariable);

// -------------------------------------------------------------------------- //

/** Notify all threads that are waiting on condition variable.
 * \brief Notify all waiting threads on condition variable.
 * \param conditionVariable Condition variable to notify threads waiting on.
 */
void alfNotifyAllConditionVariables(AlfConditionVariable* conditionVariable);

// ========================================================================== //
// Read/Write lock functions
// ========================================================================== //

/** Create a read-write lock in unlocked state.
 * \brief Create read-write lock.
 * \return Created lock or NULL on failure.
 */
AlfReadWriteLock* alfCreateReadWriteLock(void);

// -------------------------------------------------------------------------- //

/** Destroy a read-write lock.
 * \note The user is responsible for making sure that the lock is not acquired
 * by any threads when being destroyed.
 * \brief Destroy read-write lock.
 * \param lock Read-write lock to destroy.
 */
void alfDestroyReadWriteLock(AlfReadWriteLock* lock);

// -------------------------------------------------------------------------- //

/** Acquire a read-write lock in read mode. This mode allows other threads to
 * acquire the same lock in read mode. However if a thread tries to acquire the
 * lock in write mode it will block until all writers release the lock. 
 * This function will block until any potential writer thread releases the lock.
 * \brief Acquire read-write lock in read mode.
 * \param lock Lock to acquire.
 */
void alfAcquireReadLock(AlfReadWriteLock* lock);

// -------------------------------------------------------------------------- //

/** Releases a read-write lock that was previously locked in read mode.
 * \brief Release read lock.
 * \param lock Lock to release.
 */
void alfReleaseReadLock(AlfReadWriteLock* lock);

// -------------------------------------------------------------------------- //

/* Acquire a read-write lock in write mode. This mode will allow the thread to
 * have exclusive access to the lock to be able to do writing. If any other 
 * readers or writers try to acquire the lock they will block until this thread
 * releases the lock.
 * This function will block until any read or other writer releases the lock.
 * \brief Acquire read-write lock in write mode.
 * \param lock Lock to acquire.
 */
void alfAcquireWriteLock(AlfReadWriteLock* lock);

// -------------------------------------------------------------------------- //

/** Releases a read-write lock that was previously locked in write mode.
 * \brief Release write lock.
 * \param lock Lock to release.
 */
void alfReleaseWriteLock(AlfReadWriteLock* lock);

// ========================================================================== //
// TLS functions
// ========================================================================== //

/** Acquire a handle that is used to store thread-local data.
 * \note If there are not TLS handles available then the function returns NULL.
 * \brief Acquire TLS handle.
 * \return TLS handle or NULL on failure.
 */
AlfTLSHandle* alfGetTLS(void);

// -------------------------------------------------------------------------- //

/** Return a handle to a thread-local data store to the OS.
 * \note The user must not try to use the handle to store or load data after the
 * handle has been destroyed.
 * \brief Return TLS handle..
 * \param handle TLS Handle to return.
 */
void alfReturnTLS(AlfTLSHandle* handle);

// -------------------------------------------------------------------------- //

/** Store data in a thread-local store that corresponds to a TLS handle.
 * \brief Store TLS data.
 * \param handle Handle to TLS store.
 * \param data Data to store.
 */
void alfStoreTLS(AlfTLSHandle* handle, void* data);

// -------------------------------------------------------------------------- //
/** Load data from a thread-local store that corresponds to a TLS handle.
 * \brief Load TLS data.
 * \param handle Handle TLS store.
 * \return Loaded data.
 */
void* alfLoadTLS(AlfTLSHandle* handle);

// ========================================================================== //
// Atomics Functions (void*)
// ========================================================================== //

/** Atomically store a pointer in the pointer pointed to by 'pointer'.
 * \brief Atomically store pointer.
 * \param pointer Pointer to store other pointer in.
 * \param value Pointer to store.
 */
void alfAtomicStorePointer(void** pointer, void* value);

// -------------------------------------------------------------------------- //
/** Atomically load a pointer from the address specified by pointer.
 * \brief Atomically load pointer.
 * \param pointer Pointer to load.
 * \return Loaded pointer.
 */
void* alfAtomicLoadPointer(void** pointer);

// -------------------------------------------------------------------------- //
/** Atomically exchange the pointer value that is stored in 'pointer' by the
 * pointer 'value' and return the previous pointer-value stored.
 * \brief Atomically exchange pointer.
 * \param pointer Pointer to exchange value in.
 * \param value Pointer to store.
 * \return Previous pointer.
 */
void* alfAtomicExchangePointer(void** pointer, void* value);

// -------------------------------------------------------------------------- //
/** Atomically compares the comparand to the address stored in 'pointer'. If the
 * values are equal then 'value' is written into 'pointer'. In either case the
 * previous value of 'pointer' is returned.
 * \brief Atomically compare and exchange pointers.
 * \param pointer Pointer to exchange.
 * \param value Value to store if pointer is equal to comparand.
 * \param comparand Value to compare pointer with.
 * \return Previously stored pointer.
 */
void* alfAtomicCompareExchangePointer(
	void** pointer, 
	void* value, 
	void* comparand);

// ========================================================================== //
// Atomics Functions (int32_t)
// ========================================================================== //

/** Atomically store s32.
 * \brief Atomically store s32.
 * \param integer Integer to store in.
 * \param value Value to store.
 */
void alfAtomicStoreS32(int32_t* integer, int32_t value);

// -------------------------------------------------------------------------- //

/** Atomically load s32.
 * \brief Atomically load s32.
 * \param integer Integer to load.
 * \return Loaded value.
 */
int32_t alfAtomicLoadS32(int32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically exchange the s32 value in 'integer' with 'value'.
 * \brief Atomically exchange s32.
 * \param integer Integer to exchange.
 * \param value Value to exchange with.
 * \return Previous value.
 */
int32_t alfAtomicExchangeS32(int32_t* integer, int32_t value);

// -------------------------------------------------------------------------- //

/** Atomically compares the comparand to the value stored in 'integer'. If the
 * values are equal then 'value' is written into 'integer'. In either case the
 * previous value of 'integer' is returned.
 * \brief Atomically compare and exchange s32.
 * \param integer Integer to exchange.
 * \param value Value to exchange with if integer is equal to comparand.
 * \param comparand Value to compare integer with.
 * \return Previous value.
 */
int32_t alfAtomicCompareExchangeS32(
	int32_t* integer, 
	int32_t value, 
	int32_t comparand);

// -------------------------------------------------------------------------- //

/** Atomically increment the value of the s32 in 'integer'.
 * \brief Atomically increment s32.
 * \param integer Integer to increment.
 * \return Value after increment.
 */
int32_t alfAtomicIncrementS32(int32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically decrement the value of the s32 in 'integer'.
 * \brief Atomically decrement s32.
 * \param integer Integer to decrement.
 * \return Value after decrement.
 */
int32_t alfAtomicDecrementS32(int32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically add 'value' to the s32 in 'integer'.
 * \brief Atomically add s32.
 * \param integer Integer to add to.
 * \param value Value to add.
 * \return Value after addition.
 */
int32_t alfAtomicAddS32(int32_t* integer, int32_t value);

// -------------------------------------------------------------------------- //

/** Atomically subtract 'value' from the s32 in 'integer'.
 * \brief Atomically subtract s32.
 * \param integer Integer to subtract from.
 * \param value Value to subtract.
 * \return Value after subtraction.
 */
int32_t alfAtomicSubS32(int32_t* integer, int32_t value);

// ========================================================================== //
// Atomics Functions (uint32_t)
// ========================================================================== //

/** Atomically store s32.
 * \brief Atomically store s32.
 * \param integer Integer to store in.
 * \param value Value to store.
 */
void alfAtomicStoreU32(uint32_t* integer, uint32_t value);

// -------------------------------------------------------------------------- //

/** Atomically load s32.
 * \brief Atomically load s32.
 * \param integer Integer to load.
 * \return Loaded value.
 */
uint32_t alfAtomicLoadU32(uint32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically exchange the s32 value in 'integer' with 'value'.
 * \brief Atomically exchange s32.
 * \param integer Integer to exchange.
 * \param value Value to exchange with.
 * \return Previous value.
 */
uint32_t alfAtomicExchangeU32(uint32_t* integer, uint32_t value);

// -------------------------------------------------------------------------- //

/** Atomically compares the comparand to the value stored in 'integer'. If the
 * values are equal then 'value' is written into 'integer'. In either case the
 * previous value of 'integer' is returned.
 * \brief Atomically compare and exchange s32.
 * \param integer Integer to exchange.
 * \param value Value to exchange with if integer is equal to comparand.
 * \param comparand Value to compare integer with.
 * \return Previous value.
 */
uint32_t alfAtomicCompareExchangeU32(
	uint32_t* integer, 
	uint32_t value, 
	uint32_t comparand);

// -------------------------------------------------------------------------- //

/** Atomically increment the value of the s32 in 'integer'.
 * \brief Atomically increment s32.
 * \param integer Integer to increment.
 * \return Value after increment.
 */
uint32_t alfAtomicIncrementU32(uint32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically decrement the value of the s32 in 'integer'.
 * \brief Atomically decrement s32.
 * \param integer Integer to decrement.
 * \return Value after decrement.
 */
uint32_t alfAtomicDecrementU32(uint32_t* integer);

// -------------------------------------------------------------------------- //

/** Atomically add 'value' to the s32 in 'integer'.
 * \brief Atomically add s32.
 * \param integer Integer to add to.
 * \param value Value to add.
 * \return Value after addition.
 */
uint32_t alfAtomicAddU32(uint32_t* integer, uint32_t value);

// -------------------------------------------------------------------------- //

/** Atomically subtract 'value' from the s32 in 'integer'.
 * \brief Atomically subtract s32.
 * \param integer Integer to subtract from.
 * \param value Value to subtract.
 * \return Value after subtraction.
 */
uint32_t alfAtomicSubU32(uint32_t* integer, uint32_t value);

// ========================================================================== //
// Utility Functions
// ========================================================================== //

/** Returns the number of hardware threads that are available in the system.
 * \brief Returns hardware thread count.
 * \return Hardware thread count.
 */
uint32_t alfGetHardwareThreadCount(void);

// -------------------------------------------------------------------------- //

/** Returns the size of a cache line in the cache of the specified type.
 * \brief Returns cache line size.
 * \param type Type of cache.
 * \return Cache line size. -1 is returned if info could not be retrieved or the
 * system does not have the specified type of cache.
 */
int32_t alfGetCacheLineSize(AlfCache type);

// ========================================================================== //
// End of header
// ========================================================================== //

#if defined(__cplusplus)
}
#endif

#endif // ALF_THREAD_H
