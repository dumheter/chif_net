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

#include "alf_thread.h"

// ========================================================================== //
// Header Includes
// ========================================================================== //

// Standard headers
#include <memory.h>

// Platform detection
#if defined(_WIN32) || defined(_WIN64) || defined(__MINGW64__)
#	define ALF_THREAD_TARGET_WINDOWS
#	define NOMINMAX
#	define WIN32_LEAN_AND_MEAN
#	include <Windows.h>
#	include <process.h>
#elif defined(__linux__)
#	define ALF_THREAD_TARGET_LINUX
#	define ALF_THREAD_PTHREAD
#   define _GNU_SOURCE
#	include <semaphore.h>
#	include <zconf.h>
#	include <errno.h>
#	include <unistd.h>
#elif defined (__APPLE__)
#	define ALF_THREAD_TARGET_APPLE
#	if __has_feature(objc_arc)
#		define ALF_THREAD_APPLE_ARC
#	endif
#	define ALF_THREAD_PTHREAD
#	include <dispatch/dispatch.h>
#	include <errno.h>
#	include <unistd.h>
#else
UNSUPPORTED_PLATFORM
#endif

// Pthread header
#if defined(ALF_THREAD_PTHREAD)
#   include <signal.h>
#	include <pthread.h>
#endif

// ========================================================================== //
// Structures
// ========================================================================== //

typedef struct tag_AlfThread
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	/** Thread handle**/
	HANDLE handle;
#elif defined(ALF_THREAD_PTHREAD)
	/** Thread handle**/
	pthread_t handle;
#endif
	/** Thread id **/
	uint32_t id;
	/** Name of thread **/
	char* name;
	/** Whether the thread has been detached **/
	AlfBool detached;
} tag_AlfThread;

// -------------------------------------------------------------------------- //

typedef struct tag_AlfSemaphore
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	HANDLE handle;
#elif defined(ALF_THREAD_TARGET_LINUX)
	sem_t handle;
#elif defined(ALF_THREAD_TARGET_APPLE)
	dispatch_semaphore_t handle;
#endif
} tag_AlfSemaphore;

// -------------------------------------------------------------------------- //

typedef struct tag_AlfMutex
{
	/** Whether mutex supports recursive locking **/
	AlfBool recursive;
#if defined(ALF_THREAD_TARGET_WINDOWS)
	union
	{
		/** Windows slim reader-writer lock handle for non-recursive mutexes **/
		SRWLOCK srwlock;
		/** Windows critical section for recursive mutexes **/
		CRITICAL_SECTION criticalSection;
	};
#elif defined(ALF_THREAD_PTHREAD)
	/** Pthread mutex handle **/
	pthread_mutex_t handle;
	/** Pthread mutex attributes **/
	pthread_mutexattr_t attributes;
#endif
} tag_AlfMutex;

// -------------------------------------------------------------------------- //

typedef struct tag_AlfConditionVariable
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	/** Windows condition variable handle **/
	CONDITION_VARIABLE handle;
#elif defined(ALF_THREAD_PTHREAD)
	/** Pthread condition variable handle **/
	pthread_cond_t handle;
#endif
} tag_AlfConditionVariable;

// -------------------------------------------------------------------------- //

typedef struct tag_AlfReadWriteLock
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	SRWLOCK handle;
#elif defined(ALF_THREAD_PTHREAD)
	pthread_rwlock_t handle;
#endif
} tag_AlfReadWriteLock;

// -------------------------------------------------------------------------- //

typedef struct tag_AlfTLSHandle
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	DWORD handle;
#elif defined(ALF_THREAD_PTHREAD)
	pthread_key_t handle;
#endif
} tag_AlfTLSHandle;

// ========================================================================== //
// Private structures
// ========================================================================== //

/** List node structure **/
typedef struct AlfNode
{
	/** Data pointer **/
	void* data;
	/** Pointer to next node **/
	struct AlfNode* next;
} AlfNode;

// -------------------------------------------------------------------------- //

/** Global data for AlfThread library **/
typedef struct AlfGlobalData
{
	/** Mutex for global data locking **/
	AlfMutex* mutex;
	/** TLS handle for thread handle data **/
	AlfTLSHandle* handleTLS;

	/** Handle to node of first external thread **/
	AlfNode* externalThreads;
} AlfGlobalData;

// -------------------------------------------------------------------------- //

/** Internal thread data struct. Passed to thread functions **/
typedef struct
{
	/** Userdefined thread function **/
	PFN_AlfThreadFunction function;
	/** Argument to thread function **/
	void* argument;
	/** Name of thread **/
	const char* name;
	/** Thread handle pointer to store in tls **/
	AlfThread* handle;

	/** Semaphore to release when all data from this struct has is retrieved **/
	AlfSemaphore* semaphore;
	/** Whether the thread started correctly **/
	AlfBool success;
} AlfThreadData;

// ========================================================================== //
// Global data
// ========================================================================== //

/** Global data **/
static AlfGlobalData gData;

// ========================================================================== //
// Private functions
// ========================================================================== //

/** Free a thread handle **/
static void alfFreeThreadHandle(AlfThread* thread)
{
	if (thread->name)
	{
		ALF_THREAD_FREE(thread->name);
	}
	ALF_THREAD_FREE(thread);
}

// -------------------------------------------------------------------------- //

/** Add a thread that was not started by AlfThread to the list of handled
 * threads **/
static void alfAddExternalThread(AlfThread* thread)
{
	// Acquire global data mutex
	alfAcquireMutex(gData.mutex);

	// Find last node
	AlfNode** current = &gData.externalThreads;
	while (*current)
	{
		// Go to next node
		current = &(*current)->next;
	}

	// Append new node
	AlfNode* node = ALF_THREAD_ALLOC(sizeof(AlfNode));
	node->next = NULL;
	node->data = thread;
	*current = node;

	// Release global data mutex
	alfReleaseMutex(gData.mutex);
}

// ========================================================================== //
// Windows-specific functions
// ========================================================================== //

#if defined(ALF_THREAD_TARGET_WINDOWS)

/** Prototype for GetThreadDescription function **/
typedef HRESULT(*PFN_GetThreadDescription)(
	HANDLE thread_handle,
	PWSTR* thread_description);

// -------------------------------------------------------------------------- //

/** Prototype for SetThreadDescription function **/
typedef HRESULT(*PFN_SetThreadDescription)(
	HANDLE thread_handle,
	PCWSTR thread_description);

// -------------------------------------------------------------------------- //

static wchar_t* alfThreadUTF8ToUTF16(const char* string)
{
	// if the string is NULL then return
	if (!string) { return NULL; }

	// Get the byte count
	const uint64_t byteCount = strlen(string);
	if (byteCount < 1) { return NULL; }

	// Get the length of the converted string
	const int32_t convertedLength = MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		string,
		(int32_t)byteCount,
		NULL,
		0
	);

	// Assert that the string can be converted
	ALF_THREAD_ASSERT(convertedLength > 0, "Invalid UTF-8 string");

	// Allocate the utf16 string
	wchar_t* buffer = ALF_THREAD_ALLOC(sizeof(wchar_t) * (convertedLength + 1));
	buffer[convertedLength] = 0;

	// Convert the string
	MultiByteToWideChar(
		CP_UTF8,
		MB_ERR_INVALID_CHARS,
		string,
		(int32_t)byteCount,
		buffer,
		convertedLength
	);

	// Return the utf16 string
	return buffer;
}

// -------------------------------------------------------------------------- //

static uint32_t alfWin32ThreadStart(void* argument)
{
	// Cast the argument to thread data and retrieve functiona and argument
	const AlfThreadData* data = (AlfThreadData*)argument;
	const PFN_AlfThreadFunction function = data->function;
	void* user_argument = data->argument;

	// Store thread handle
	alfStoreTLS(gData.handleTLS, data->handle);

	// Set the thread name
	alfSetThreadName(data->name);

	// Release the semaphore
	alfReleaseSemaphore(data->semaphore);

	// Call the function
	const uint32_t result = function(user_argument);

	// Cleanup thread if detached
	AlfThread* thread = alfThisThread();
	if (thread->detached)
	{
		alfFreeThreadHandle(thread);
	}

	// Return result and end thread
	return result;
}
#endif // defined(ALF_THREAD_TARGET_WINDOWS)

// ========================================================================== //
// Pthread-specific functions
// ========================================================================== //

/** Thread start function **/
#if defined(ALF_THREAD_PTHREAD)

void _alfMillisecondsToTimespec(uint32_t ms, struct timespec* time)
{
	time->tv_sec = ms / 1000;
	time->tv_nsec = (ms % 1000) * 1000000;
}

// -------------------------------------------------------------------------- //

static void* _alfPthreadThreadStart(void* argument)
{
	// Cast the argument to thread data and retrieve functiona and argument
	const AlfThreadData* data = (AlfThreadData*)argument;
	const PFN_AlfThreadFunction function = data->function;
	void* user_argument = data->argument;

	// Store thread handle
	alfStoreTLS(gData.handleTLS, data->handle);

	// Set the thread name
	alfSetThreadName(data->name);

	// Release the semaphore
	alfReleaseSemaphore(data->semaphore);

	// Call the function
	const uint32_t result = function(user_argument);

	// Cleanup thread if detached
	AlfThread* thread = alfThisThread();
	if (thread->detached)
	{
		alfFreeThreadHandle(thread);
	}

	// Return result and end thread
	return (void*)(uint64_t)result;
}

#endif // defined(ALF_THREAD_PTHREAD)

// ========================================================================== //
// Thread functions
// ========================================================================== //

void alfThreadStartup(void)
{
	gData.handleTLS = alfGetTLS();
	gData.mutex = alfCreateMutex(ALF_FALSE);
}

// -------------------------------------------------------------------------- //

void alfThreadShutdown()
{
	// Free all external node handles
	AlfNode* current = gData.externalThreads;
	while (current)
	{
		// Store old node and step to next
		AlfNode* old = current;
		current = current->next;

		// Cleanup the thread handle
		alfFreeThreadHandle((AlfThread*)old->data);

		// Free old node
		ALF_THREAD_FREE(old);
	}

	// Deallocate tls handles
	alfReturnTLS(gData.handleTLS);

	// Delete mutex
	alfDeleteMutex(gData.mutex);
}

// -------------------------------------------------------------------------- //

AlfThread* alfCreateThread(
	PFN_AlfThreadFunction function,
	void* argument)
{
	return alfCreateThreadNamed(function, argument, NULL);
}

// -------------------------------------------------------------------------- //

AlfThread* alfCreateThreadNamed(
	PFN_AlfThreadFunction function,
	void* argument,
	const char* name)
{
	// Allocate thread object
	AlfThread* thread = ALF_THREAD_ALLOC(sizeof(AlfThread));
	if (!thread) { return NULL; }

	// Clear thread handle
	memset(thread, 0, sizeof(AlfThread));

	// Setup thread data
	AlfThreadData data;
	data.function = function;
	data.argument = argument;
	data.name = name;
	data.handle = thread;
	data.semaphore = alfCreateSemaphore(0);
	data.success = ALF_TRUE;

	// Check if the semaphore is valid
	if (!data.semaphore)
	{
		ALF_THREAD_FREE(thread);
		return NULL;
	}

#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Start the thread
	uint32_t id;
	const HANDLE handle = (HANDLE)_beginthreadex(
		NULL,
		0,
		alfWin32ThreadStart,
		(void*)&data,
		0,
		&id
	);

	// Check if handle is valid
	if (handle == INVALID_HANDLE_VALUE)
	{
		// Free thread object and return
		ALF_THREAD_FREE(thread);
		return NULL;
	}
#elif defined(ALF_THREAD_PTHREAD)
	// Start thread
	pthread_t handle;
	const int result = pthread_create(
		&handle,
		NULL,
		_alfPthreadThreadStart,
		(void*)&data
	);

	// Check result
	if (result != 0)
	{
		// Free thread object and return
		ALF_THREAD_FREE(thread);
		return NULL;
	}

	// Get id
	uint32_t id = (uint32_t)(u_int64_t)handle;
#endif

	// Wait for and then destroy semaphore
	alfAcquireSemaphore(data.semaphore);
	alfDeleteSemaphore(data.semaphore);

	// Assign handle and id
	thread->handle = handle;
	thread->id = id;

	// Setup other thread properties
	thread->detached = ALF_FALSE;

	// Check for errors during thread initialization
	if (!data.success)
	{
		// Join the thread
		alfJoinThread(thread);
	}

	// Return the thread
	return thread;
}

// -------------------------------------------------------------------------- //

AlfThread* alfThisThread()
{
	// Get thread
	AlfThread* thread = (AlfThread*)alfLoadTLS(gData.handleTLS);

	// Check if handle is valid
	if (!thread)
	{
		// Allocate a new handle
		thread = ALF_THREAD_ALLOC(sizeof(AlfThread));
		if (!thread) { return NULL; }

		// Set thread-handle data
#if defined(ALF_THREAD_TARGET_WINDOWS)
		thread->handle = (HANDLE)GetCurrentThread();
		thread->id = (uint32_t)GetCurrentThreadId();
		thread->name = NULL;
		thread->detached = ALF_FALSE;
#elif defined(ALF_THREAD_PTHREAD)
		thread->handle = pthread_self();
		thread->id = (uint32_t)(uint64_t)thread->handle;
		thread->name = NULL;
		thread->detached = ALF_FALSE;
#endif

		// Setup thread handle
		alfStoreTLS(gData.handleTLS, thread);

		// Add external thread handle
		alfAddExternalThread(thread);
	}

	// Return the thread object
	return thread;
}

// -------------------------------------------------------------------------- //

uint32_t alfJoinThread(AlfThread* thread)
{
	// Exit code
	uint32_t exitCode;

#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Wait for thread
	WaitForSingleObject(thread->handle, INFINITE);
	DWORD _exitCode;
	GetExitCodeThread(thread->handle, &_exitCode);
	exitCode = _exitCode;

	// Close handle to the thread
	CloseHandle(thread->handle);
#elif defined(ALF_THREAD_PTHREAD)
	// Join thread
	void* result;
	pthread_join(thread->handle, &result);
	exitCode = (uint32_t)((uint64_t)result);
#endif

	// Free the thread handle
	alfFreeThreadHandle(thread);

	// Return exit code
	return exitCode;
}

// -------------------------------------------------------------------------- //

AlfBool alfJoinThreadTry(AlfThread* thread, uint32_t* exitCodeOut)
{
	uint32_t exitCode;

#if defined(ALF_THREAD_TARGET_WINDOWS)
	const DWORD result = WaitForSingleObject(thread->handle, 0);
	ALF_THREAD_ASSERT(result != WAIT_FAILED && result != WAIT_ABANDONED,
		"Failed to try to join thread");
	if (result == WAIT_TIMEOUT)
	{
		*exitCodeOut = 0;
		return ALF_FALSE;
	}
	DWORD _exitCode;
	GetExitCodeThread(thread->handle, &_exitCode);
	exitCode = _exitCode;
	CloseHandle(thread->handle);
#elif defined(ALF_THREAD_TARGET_APPLE)
	ALF_THREAD_ASSERT(0, "Apple systems currently does not support try-join");
#elif defined(ALF_THREAD_PTHREAD)
    ALF_THREAD_ASSERT(0, "Linux systems currently does not support try-join");
	/* void* result; */
	/* int result = pthread_tryjoin_np(thread->handle, &result); */
	/* if (result == EBUSY) */
	/* { */
	/* 	*exitCodeOut = 0; */
	/* 	return ALF_FALSE; */
	/* } */
	/* exitCode = (uint32_t)((uint64_t)result); */
#endif

	alfFreeThreadHandle(thread);

	*exitCodeOut = exitCode;
	return ALF_TRUE;
}

// -------------------------------------------------------------------------- //

void alfDetachThread(AlfThread* thread)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const BOOL result = CloseHandle(thread->handle);
	ALF_THREAD_ASSERT(result != FALSE, "Failed to detach thread");
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_detach(thread->handle);
	ALF_THREAD_ASSERT(result == 0, "Failed to detach thread");
#endif

	thread->detached = ALF_TRUE;
}

// -------------------------------------------------------------------------- //

void alfKillThread(AlfThread* thread)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const BOOL result = TerminateThread(thread->handle, 0);
	ALF_THREAD_ASSERT(result != 0, "Failed to kill thread");
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_kill(thread->handle, 0);
	ALF_THREAD_ASSERT(result == 0, "Failed to kill thread");
#endif
}

// -------------------------------------------------------------------------- //

void alfExitThread(uint32_t exitCode)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Cleanup thread if detached
	AlfThread* thread = alfThisThread();
	if (thread->detached)
	{
		alfFreeThreadHandle(thread);
	}

	// Exit thread
	_endthreadex(exitCode);
#elif defined(ALF_THREAD_PTHREAD)
	pthread_exit((void*)((uint64_t)exitCode));
#endif
}

// -------------------------------------------------------------------------- //

void alfYieldThread()
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	SwitchToThread();
#elif defined(ALF_THREAD_PTHREAD)
	int result = sched_yield();
	ALF_THREAD_ASSERT(result == 0, "Failed to yield thread");
#endif
}

// -------------------------------------------------------------------------- //

AlfBool alfSetThreadPriority(
	AlfThread* thread,
	AlfThreadPriority priority)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	int32_t prio = THREAD_PRIORITY_NORMAL;
	switch (priority)
	{
		case ALF_THREAD_PRIORITY_LOWEST:
		{
			prio = THREAD_PRIORITY_LOWEST; break;
		}
		case ALF_THREAD_PRIORITY_LOW:
		{
			prio = THREAD_PRIORITY_BELOW_NORMAL; break;
		}
		case ALF_THREAD_PRIORITY_HIGH:
		{
			prio = THREAD_PRIORITY_ABOVE_NORMAL; break;
		}
		case ALF_THREAD_PRIORITY_HIGHEST:
		{
			prio = THREAD_PRIORITY_HIGHEST; break;
		}
		case ALF_THREAD_PRIORITY_CRITICAL:
		{
			prio = THREAD_PRIORITY_TIME_CRITICAL; break;
		}
		case ALF_THREAD_PRIORITY_NORMAL:
		{
			break;
		}
		default:
		{
			return ALF_FALSE;
		}
	}
	const BOOL result = SetThreadPriority(
		thread->handle,
		prio
	);
	return result != 0;
#elif defined(ALF_THREAD_TARGET_APPLE)
	return ALF_FALSE;
#elif defined(ALF_THREAD_PTHREAD)
	int prio = 0;
	switch (priority)
	{
		case ALF_THREAD_PRIORITY_LOWEST: { prio = -2; break; }
		case ALF_THREAD_PRIORITY_LOW: { prio = -1; break; }
		case ALF_THREAD_PRIORITY_HIGH: { prio = 1; break; }
		case ALF_THREAD_PRIORITY_HIGHEST: { prio = 2; break; }
		case ALF_THREAD_PRIORITY_CRITICAL: { prio = 3; break; }
		default: break;
	}
	int result = pthread_setschedprio(thread->handle, prio);

	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid thread priority. This is an internal error");
	ALF_THREAD_ASSERT(result != ESRCH,
		"Invalid thread handle. This is an internal error");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to set thread priority");
	return ALF_TRUE;
#endif
}

// -------------------------------------------------------------------------- //

void alfSleepThread(uint64_t milliseconds)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	Sleep((DWORD)milliseconds);
#elif defined(ALF_THREAD_PTHREAD)
	struct timespec time;
	_alfMillisecondsToTimespec((uint32_t)milliseconds, &time);
	nanosleep(&time, NULL);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfGetThreadID(AlfThread* thread)
{
	return thread->id;
}

// -------------------------------------------------------------------------- //

const char* alfGetThreadName()
{
	// Get thread handle
	AlfThread* thread = alfThisThread();

	// Return 'Unknown' for threads with no set name
	if (!thread->name)
	{
		alfSetThreadName(ALF_DEFAULT_THREAD_NAME);
	}

	// Return the thread name
	return thread->name;
}

// -------------------------------------------------------------------------- //

AlfBool alfSetThreadName(const char* name)
{
	// If name is NULL then set name to Unknown
	if (!name)
	{
		return alfSetThreadName(ALF_DEFAULT_THREAD_NAME);
	}

	// Get thread handle
	AlfThread* thread = alfThisThread();

	// Free old name
	if (thread->name)
	{
		ALF_THREAD_FREE(thread->name);
		thread->name = NULL;
	}

	// Copy name into handle if not NULL
	const size_t length = strlen(name);
	thread->name = (char*)ALF_THREAD_ALLOC(length + 1);
	memcpy(thread->name, name, length + 1);

#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Get the address to 'SetThreadDescription'.
	const PFN_SetThreadDescription pSetThreadDescription =
		(PFN_SetThreadDescription)GetProcAddress(
			GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription");

	// Use the function if available
	if (pSetThreadDescription)
	{
		// Convert name to utf16
		wchar_t* _name = alfThreadUTF8ToUTF16(name);

		// Set the name of the thread
		const HRESULT result = pSetThreadDescription(GetCurrentThread(), _name);
		if (!SUCCEEDED(result))
		{
			return ALF_FALSE;
		}

		// Free the converted string
		ALF_THREAD_FREE(_name);
	}
	else
	{
		// TODO(Filip Bj�rklund): Implement using exceptions
	}
#elif defined(ALF_THREAD_PTHREAD) && !defined(__linux__)
	// Create temp name if longer than 15 characters
	if (length > 15)
	{
		// Use temp name
		char temp_name[16];
		temp_name[15] = 0;
		memcpy(temp_name, name, 15);
		// TODO(Filip Bj�rklund): Don't cut UTF-8 codepoints in half!

		// Set the thread name
		pthread_setname_np(temp_name);
	}
	else
	{
		// Set the thread name
		pthread_setname_np(name);
	}
#endif

	return ALF_TRUE;
}

// ========================================================================== //
// Semaphore functions
// ========================================================================== //

AlfSemaphore* alfCreateSemaphore(uint64_t initialValue)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Create the semaphore handle
	const HANDLE handle = CreateSemaphoreExW(
		NULL,
		(LONG)initialValue,
		(LONG)LONG_MAX,
		NULL,
		0,
		SYNCHRONIZE | SEMAPHORE_MODIFY_STATE
	);
	if (!handle)
	{
		return NULL;
	}
#elif defined(ALF_THREAD_TARGET_LINUX)
	ALF_THREAD_ASSERT(initialValue < SEM_VALUE_MAX,
		"Initial semaphore value exceeds SEM_VALUE_MAX for pthread "
		"implementation");
	sem_t handle;
	const int result = sem_init(&handle, 0, (unsigned int)initialValue);
	if (result != 0)
	{
		return NULL;
	}
#elif defined(ALF_THREAD_TARGET_APPLE)
	dispatch_semaphore_t handle = dispatch_semaphore_create(initialValue);
#endif

	AlfSemaphore* semaphore = ALF_THREAD_ALLOC(sizeof(AlfSemaphore));
	semaphore->handle = handle;
	return semaphore;
}

// -------------------------------------------------------------------------- //

void alfDeleteSemaphore(AlfSemaphore* semaphore)
{
	if (!semaphore) { return; }

#if defined(ALF_THREAD_TARGET_WINDOWS)
	CloseHandle(semaphore->handle);
#elif defined(ALF_THREAD_TARGET_LINUX)
	sem_destroy(&semaphore->handle);
#elif defined(ALF_THREAD_TARGET_APPLE) && !defined(ALF_THREAD_APPLE_ARC)
	dispatch_release(semaphore->handle);
#endif

	ALF_THREAD_FREE(semaphore);
}

// -------------------------------------------------------------------------- //

void alfAcquireSemaphore(AlfSemaphore* semaphore)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Wait for semaphore
	const DWORD result =
		WaitForSingleObjectEx(semaphore->handle, INFINITE, FALSE);
	ALF_THREAD_ASSERT(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT,
		"Failed to acquire semaphore");
#elif defined(ALF_THREAD_TARGET_LINUX)
	const int result = sem_wait(&semaphore->handle);
	if (result == -1)
	{
		int error = errno;
		ALF_THREAD_ASSERT(error != EINVAL,
			"Invalid semaphore handle. This is an internal error");
		ALF_THREAD_ASSERT(result == 0,
			"Failed to release semaphore");
	}
#elif defined(ALF_THREAD_TARGET_APPLE)
	long result =
		dispatch_semaphore_wait(semaphore->handle, DISPATCH_TIME_FOREVER);
	ALF_THREAD_ASSERT(result == 0,"Failed to acquire semaphore");
#endif
}

// -------------------------------------------------------------------------- //

AlfBool alfAcquireSemaphoreTimed(
	AlfSemaphore* semaphore,
	uint64_t milliseconds)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Wait for semaphore
	const DWORD result =
		WaitForSingleObjectEx(semaphore->handle, (DWORD)milliseconds, FALSE);
	ALF_THREAD_ASSERT(result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT,
		"Failed to acquire semaphore with timeout");
	return result == WAIT_OBJECT_0;
#elif defined(ALF_THREAD_TARGET_LINUX)
	int result;
	if (milliseconds == 0)
	{
		result = sem_trywait(&semaphore->handle);
	}
	else
	{
		struct timespec timeout;
		_alfMillisecondsToTimespec(milliseconds, &timeout);
		result = sem_timedwait(&semaphore->handle, &timeout);
	}

	if (result == -1)
	{
		int error = errno;
		ALF_THREAD_ASSERT(error != EINVAL,
			"Invalid semaphore handle. This is an internal error");
		ALF_THREAD_ASSERT(error == EAGAIN || error == ETIMEDOUT,
			"Failed to release semaphore");
		return ALF_FALSE;
	}
	return ALF_TRUE;
#elif defined(ALF_THREAD_TARGET_APPLE)
	dispatch_time_t timeout = dispatch_time(
		DISPATCH_TIME_NOW,
		milliseconds * 1000000
	);
	long result = dispatch_semaphore_wait(semaphore->handle, timeout);
	return result == 0;
#endif
}

// -------------------------------------------------------------------------- //

AlfBool alfAcquireSemaphoreTry(AlfSemaphore* semaphore)
{
	// Do a timed acquire with immediate timeout
	return
		alfAcquireSemaphoreTimed(semaphore, ALF_THREAD_IMMEDIATELY) == ALF_TRUE;
}

// -------------------------------------------------------------------------- //

void alfReleaseSemaphore(AlfSemaphore* semaphore)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Release semaphore
	const BOOL result = ReleaseSemaphore(semaphore->handle, (LONG)1, NULL);
	ALF_THREAD_ASSERT(result != 0, "Failed to release semaphore");
#elif defined(ALF_THREAD_TARGET_LINUX)
	// Post semaphore
	const int result = sem_post(&semaphore->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid semaphore handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EOVERFLOW,
		"Maximum semaphore value would be exceeded");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to release semaphore");
#elif defined(ALF_THREAD_TARGET_APPLE)
	dispatch_semaphore_signal(semaphore->handle);
#endif
}

// ========================================================================== //
// Mutex functions
// ========================================================================== //

AlfMutex* alfCreateMutex(AlfBool recursive)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	AlfMutex handle;
	if (recursive) { InitializeCriticalSection(&handle.criticalSection); }
	else { InitializeSRWLock(&handle.srwlock); }
#elif defined(ALF_THREAD_PTHREAD)
	AlfMutex handle;

	int result = pthread_mutexattr_init(&handle.attributes);
	if (result != 0) { return NULL; }

	pthread_mutexattr_settype(
		&handle.attributes,
		recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_ERRORCHECK
	);

	result = pthread_mutex_init(&handle.handle, &handle.attributes);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid pthread mutex attribute specified during creation");
	if (result != 0) { return NULL; }
#endif

	// Allocate mutex object
	AlfMutex* mutex = ALF_THREAD_ALLOC(sizeof(AlfMutex));
	handle.recursive = recursive;
	*mutex = handle;

	// Return the mutex
	return mutex;
}

// -------------------------------------------------------------------------- //

void alfDeleteMutex(AlfMutex* mutex)
{
	// Return immediately if the handle is NULL
	if (!mutex) { return; }

#if defined(ALF_THREAD_TARGET_WINDOWS)
	if (mutex->recursive)
	{
		DeleteCriticalSection(&mutex->criticalSection);
	}
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_mutex_destroy(&mutex->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid mutex handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EBUSY,
		"Mutex cannot be destroyed while still in use");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to destroy mutex");

	result = pthread_mutexattr_destroy(&mutex->attributes);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid pthread mutex attribute object");
#endif

	// Free the mutex object
	ALF_THREAD_FREE(mutex);
}

// -------------------------------------------------------------------------- //

void alfAcquireMutex(AlfMutex* mutex)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	if (mutex->recursive) { EnterCriticalSection(&mutex->criticalSection); }
	else { AcquireSRWLockExclusive(&mutex->srwlock); }
#elif defined(ALF_THREAD_PTHREAD)
	int result;
	do
	{
		result = pthread_mutex_lock(&mutex->handle);
	} while (result == EBUSY);

	ALF_THREAD_ASSERT(result != EAGAIN,
		"Recursive mutex is locked too many times");
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid mutex handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EDEADLK,
		"Acquiring mutex resulted in deadlock");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to acquire mutex");
#endif
}

// -------------------------------------------------------------------------- //

AlfBool alfAcquireMutexTry(AlfMutex* mutex)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	if (mutex->recursive)
	{
		return TryEnterCriticalSection(&mutex->criticalSection) != 0;
	}
	return TryAcquireSRWLockExclusive(&mutex->srwlock) != 0;
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_mutex_trylock(&mutex->handle);
	return result == 0;
#endif
}

// -------------------------------------------------------------------------- //

void alfReleaseMutex(AlfMutex* mutex)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	if (mutex->recursive) { LeaveCriticalSection(&mutex->criticalSection); }
	else { ReleaseSRWLockExclusive(&mutex->srwlock); }
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_mutex_unlock(&mutex->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid mutex handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EPERM,
		"Cannot release mutex not owned by the calling thread");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to release mutex");
#endif
}

// -------------------------------------------------------------------------- //

AlfBool alfIsMutexRecursive(AlfMutex* mutex)
{
	return mutex->recursive;
}

// ========================================================================== //
// Condition variable functions
// ========================================================================== //

AlfConditionVariable* alfCreateConditionVariable()
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	CONDITION_VARIABLE handle;
	InitializeConditionVariable(&handle);
#elif defined(ALF_THREAD_PTHREAD)
	pthread_cond_t handle;
	int result = pthread_cond_init(&handle, NULL);
	if (result != 0)
	{
		return NULL;
	}
#endif

	AlfConditionVariable* conditionVariable =
		ALF_THREAD_ALLOC(sizeof(AlfConditionVariable));
	conditionVariable->handle = handle;
	return conditionVariable;
}

// -------------------------------------------------------------------------- //

void alfDeleteConditionVariable(AlfConditionVariable* conditionVariable)
{
	if (!conditionVariable) { return; }

#if defined(ALF_THREAD_PTHREAD)
	int result = pthread_cond_destroy(&conditionVariable->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid condition variable handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EBUSY,
		"Condition variable cannot be destroyed while still being waited on");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to destroy condition variable");
#endif

	// Free condition variable object
	ALF_THREAD_FREE(conditionVariable);
}

// -------------------------------------------------------------------------- //

void alfWaitConditionVariable(
	AlfConditionVariable* conditionVariable,
	AlfMutex* mutex)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	BOOL result;
	if (mutex->recursive)
	{
		result = SleepConditionVariableCS(
			&conditionVariable->handle,
			&mutex->criticalSection,
			INFINITE
		);
	}
	else
	{
		result = SleepConditionVariableSRW(
			&conditionVariable->handle,
			&mutex->srwlock,
			INFINITE,
			0
		);
	}
	ALF_THREAD_ASSERT(result != 0, "Failed to wait on condition variable");
#elif defined(ALF_THREAD_PTHREAD)
	int result =
		pthread_cond_wait(&conditionVariable->handle, &mutex->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid condition variable handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EPERM,
		"Cannot wait on condition variable by specifying a mutex that is not "
		"owned by the calling thread");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to wait on condition variable");
#endif
}

// -------------------------------------------------------------------------- //

void alfWaitConditionVariablePredicate(
	AlfConditionVariable* conditionVariable,
	AlfMutex* mutex,
	PFN_AlfPredicate predicate,
	void* predicateArgument)
{
	while (!predicate(predicateArgument))
	{
		alfWaitConditionVariable(conditionVariable, mutex);
	}
}

// -------------------------------------------------------------------------- //

void alfNotifyConditionVariable(AlfConditionVariable* conditionVariable)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	WakeConditionVariable(&conditionVariable->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_cond_signal(&conditionVariable->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid condition variable handle. This is an internal error");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to notify condition variable");
#endif
}

// -------------------------------------------------------------------------- //

void alfNotifyAllConditionVariables(
	AlfConditionVariable* conditionVariable)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	WakeAllConditionVariable(&conditionVariable->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_cond_broadcast(&conditionVariable->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid condition variable handle. This is an internal error");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to notify condition variable");
#endif
}

// ========================================================================== //
// Read/Write lock functions
// ========================================================================== //

AlfReadWriteLock* alfCreateReadWriteLock()
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	SRWLOCK handle;
	InitializeSRWLock(&handle);
#elif defined(ALF_THREAD_PTHREAD)
	pthread_rwlock_t handle;
	int32_t result = pthread_rwlock_init(&handle, NULL);
	if (result != 0)
	{
		return NULL;
	}
#endif

	AlfReadWriteLock* lock = (AlfReadWriteLock*)ALF_THREAD_ALLOC(
		sizeof(AlfReadWriteLock));
	lock->handle = handle;
	return lock;
}

// -------------------------------------------------------------------------- //

void alfDestroyReadWriteLock(AlfReadWriteLock* lock)
{
	if (!lock) { return; }

#if defined(ALF_THREAD_PTHREAD)
	int32_t result = pthread_rwlock_destroy(&lock->handle);

	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid read-write lock handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EBUSY,
		"Read-write lock cannot be destroyed while still in use");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to destroy read-write lock");

#endif

	ALF_THREAD_FREE(lock);
}

// -------------------------------------------------------------------------- //

void alfAcquireReadLock(AlfReadWriteLock* lock)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	AcquireSRWLockShared(&lock->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int32_t result = pthread_rwlock_rdlock(&lock->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid read-write lock handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EDEADLK,
		"Read-write lock was acquired in read mode from the same thread "
		"recursively. This means that the thread has deadlocked");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to acquire read-write lock in read mode");
#endif
}

// -------------------------------------------------------------------------- //

void alfReleaseReadLock(AlfReadWriteLock* lock)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	ReleaseSRWLockShared(&lock->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int32_t result = pthread_rwlock_unlock(&lock->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid read-write lock handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EPERM,
		"Read-write lock cannot be released when not acquired by thread");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to release read-write lock from read mode");
#endif
}

// -------------------------------------------------------------------------- //

void alfAcquireWriteLock(AlfReadWriteLock* lock)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	AcquireSRWLockExclusive(&lock->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int32_t result = pthread_rwlock_wrlock(&lock->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid read-write lock handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EDEADLK,
		"Read-write lock was acquired in write mode from the same thread "
		"recursively. This means that the thread has deadlocked");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to acquire read-write lock in write mode");
#endif
}

// -------------------------------------------------------------------------- //

void alfReleaseWriteLock(AlfReadWriteLock* lock)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	ReleaseSRWLockExclusive(&lock->handle);
#elif defined(ALF_THREAD_PTHREAD)
	int32_t result = pthread_rwlock_unlock(&lock->handle);
	ALF_THREAD_ASSERT(result != EINVAL,
		"Invalid read-write lock handle. This is an internal error");
	ALF_THREAD_ASSERT(result != EPERM,
		"Read-write lock cannot be released when not acquired by thread");
	ALF_THREAD_ASSERT(result == 0,
		"Failed to release read-write lock from read mode");
#endif
}

// ========================================================================== //
// TLS functions
// ========================================================================== //

AlfTLSHandle* alfGetTLS()
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	// Allocate tls
	const DWORD tls = TlsAlloc();
	if (tls == TLS_OUT_OF_INDEXES)
	{
		return NULL;
	}
#elif defined(ALF_THREAD_PTHREAD)
	// Create tls key
	pthread_key_t tls;
	int result = pthread_key_create(&tls, NULL);
	if (result != 0)
	{
		return NULL;
	}
#endif

	// Allocate handle
	AlfTLSHandle* handle = ALF_THREAD_ALLOC(sizeof(AlfTLSHandle));
	if (!handle) { return NULL; }
	handle->handle = tls;
	return handle;
}

// -------------------------------------------------------------------------- //

void alfReturnTLS(AlfTLSHandle* handle)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const BOOL result = TlsFree(handle->handle);
	ALF_THREAD_ASSERT(result != 0, "Failed to return TLS handle");
#elif defined(ALF_THREAD_PTHREAD)
	int result = pthread_key_delete(handle->handle);
	ALF_THREAD_ASSERT(result == 0, "Failed to return TLS handle");
#endif

	// Free handle
	ALF_THREAD_FREE(handle);
}

// -------------------------------------------------------------------------- //

void alfStoreTLS(AlfTLSHandle* handle, void* data)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const BOOL result = TlsSetValue(handle->handle, data);
	ALF_THREAD_ASSERT(result != 0, "Failed to store data in TLS");
#elif defined(ALF_THREAD_PTHREAD)
	pthread_setspecific(handle->handle, data);
#endif
}

// -------------------------------------------------------------------------- //

void* alfLoadTLS(AlfTLSHandle* handle)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return TlsGetValue(handle->handle);
#elif defined(ALF_THREAD_PTHREAD)
	return pthread_getspecific(handle->handle);
#endif
}


// ========================================================================== //
// Atomics Functions (void*)
// ========================================================================== //

void alfAtomicStorePointer(void** pointer, void* value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	InterlockedExchangePointer(
		(volatile PVOID*)pointer,
		(PVOID)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_store_n(
		pointer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

void* alfAtomicLoadPointer(void** pointer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedCompareExchangePointer(
		(volatile PVOID*)pointer,
		(PVOID)NULL, (PVOID)NULL
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (void*)__atomic_load_n(
		pointer,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

void* alfAtomicExchangePointer(void** pointer, void* value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedExchangePointer(
		(volatile PVOID*)pointer,
		(PVOID)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (void*)__atomic_exchange_n(
		pointer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

void* alfAtomicCompareExchangePointer(
	void** pointer,
	void* value,
	void* comparand)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return (PVOID)InterlockedCompareExchangePointer(
		(volatile PVOID*)pointer,
		(PVOID)value,
		(PVOID)comparand
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_compare_exchange_n(
		pointer,
		&comparand,
		value,
		ALF_FALSE,
		__ATOMIC_SEQ_CST,
		__ATOMIC_SEQ_CST
	);
	return comparand;
#endif
}

// ========================================================================== //
// Atomics Functions (int32_t)
// ========================================================================== //

void alfAtomicStoreS32(int32_t* integer, int32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	InterlockedExchange(
		(volatile LONG*)integer,
		(LONG)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_store_n(integer, value, __ATOMIC_SEQ_CST);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicLoadS32(int32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedCompareExchange(
		(volatile LONG*)integer,
		(LONG)0, (LONG)0
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_load_n(integer, __ATOMIC_SEQ_CST);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicExchangeS32(int32_t* integer, int32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return (uint32_t)InterlockedExchange(
		(volatile LONG*)integer,
		(LONG)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_exchange_n(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicCompareExchangeS32(
	int32_t* integer,
	int32_t value,
	int32_t comparand)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return (uint32_t)InterlockedCompareExchange(
		(volatile LONG*)integer,
		(LONG)value,
		(LONG)comparand
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_compare_exchange_n(
		integer,
		&comparand,
		value,
		ALF_FALSE,
		__ATOMIC_SEQ_CST,
		__ATOMIC_SEQ_CST
	);
	return comparand;
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicIncrementS32(int32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedIncrement((volatile LONG*)integer);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_add_fetch(
		integer,
		1,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicDecrementS32(int32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedDecrement((volatile LONG*)integer);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_sub_fetch(
		integer,
		1,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicAddS32(int32_t* integer, int32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const int32_t previous = InterlockedExchangeAdd(
		(volatile LONG*)integer,
		(LONG)value
	);
	return previous + value;
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_add_fetch(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfAtomicSubS32(int32_t* integer, int32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const int32_t previous = InterlockedExchangeAdd(
		(volatile LONG*)integer,
		-((LONG)value)
	);
	return previous - value;
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (int32_t)__atomic_sub_fetch(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// ========================================================================== //
// Atomics Functions (uint32_t)
// ========================================================================== //

void alfAtomicStoreU32(uint32_t* integer, uint32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	InterlockedExchange(
		(volatile LONG*)integer,
		(LONG)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_store_n(integer, value, __ATOMIC_SEQ_CST);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicLoadU32(uint32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedCompareExchange(
		(volatile LONG*)integer,
		(LONG)0, (LONG)0
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_load_n(integer, __ATOMIC_SEQ_CST);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicExchangeU32(uint32_t* integer, uint32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return (uint32_t)InterlockedExchange(
		(volatile LONG*)integer,
		(LONG)value
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_exchange_n(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicCompareExchangeU32(
	uint32_t* integer,
	uint32_t value,
	uint32_t comparand)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return (uint32_t)InterlockedCompareExchange(
		(volatile LONG*)integer,
		(LONG)value,
		(LONG)comparand
	);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	__atomic_compare_exchange_n(
		integer,
		&comparand,
		value,
		ALF_FALSE,
		__ATOMIC_SEQ_CST,
		__ATOMIC_SEQ_CST
	);
	return comparand;
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicIncrementU32(uint32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedIncrement((volatile LONG*)integer);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_add_fetch(
		integer,
		1,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicDecrementU32(uint32_t* integer)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	return InterlockedDecrement((volatile LONG*)integer);
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_sub_fetch(
		integer,
		1,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicAddU32(uint32_t* integer, uint32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const uint32_t previous = InterlockedExchangeAdd(
		(volatile LONG*)integer,
		(LONG)value
	);
	return previous + value;
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_add_fetch(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// -------------------------------------------------------------------------- //

uint32_t alfAtomicSubU32(uint32_t* integer, uint32_t value)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	const uint32_t previous = InterlockedExchangeAdd(
		(volatile LONG*)integer,
		-((LONG)value)
	);
	return previous - value;
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)__atomic_sub_fetch(
		integer,
		value,
		__ATOMIC_SEQ_CST
	);
#endif
}

// ========================================================================== //
// Utility Functions
// ========================================================================== //

uint32_t alfGetHardwareThreadCount()
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return (uint32_t)info.dwNumberOfProcessors;
#elif defined(ALF_THREAD_TARGET_LINUX) || defined(ALF_THREAD_TARGET_APPLE)
	return (uint32_t)sysconf(_SC_NPROCESSORS_CONF);
#endif
}

// -------------------------------------------------------------------------- //

int32_t alfGetCacheLineSize(AlfCache type)
{
#if defined(ALF_THREAD_TARGET_WINDOWS)
	DWORD infoSize;
	GetLogicalProcessorInformation(NULL, &infoSize);
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfos =
		(SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)ALF_THREAD_ALLOC(infoSize);
	GetLogicalProcessorInformation(procInfos, &infoSize);
	const DWORD procInfoCount = infoSize /
		sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

	uint32_t cacheLineSize = -1;
	for (DWORD i = 0; i < procInfoCount; i++)
	{
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfo = &procInfos[i];
		const AlfBool isSharedCache = procInfo->Relationship == RelationCache;
		const BYTE cacheLevel = procInfo->Cache.Level;
		const AlfBool isDataCache = procInfo->Cache.Type == CacheData;
		const AlfBool isInstrCache = procInfo->Cache.Type == CacheInstruction;
		const AlfBool isUnifiedCache = procInfo->Cache.Type == CacheUnified;

		if (type == ALF_CACHE_L1D &&
			isSharedCache &&
			cacheLevel == 1 &&
			isDataCache)
		{
			cacheLineSize = procInfo->Cache.LineSize;
			break;
		}
		if (type == ALF_CACHE_L1I &&
			isSharedCache &&
			cacheLevel == 1 &&
			isInstrCache)
		{
			cacheLineSize = procInfo->Cache.LineSize;
			break;
		}
		if (type == ALF_CACHE_L2 &&
			isSharedCache &&
			cacheLevel == 2 &&
			isUnifiedCache)
		{
			cacheLineSize = procInfo->Cache.LineSize;
			break;
		}
		if (type == ALF_CACHE_L3 &&
			isSharedCache &&
			cacheLevel == 3 &&
			isUnifiedCache)
		{
			cacheLineSize = procInfo->Cache.LineSize;
			break;
		}
		if (type == ALF_CACHE_L4 &&
			isSharedCache &&
			cacheLevel == 4 &&
			isUnifiedCache)
		{
			cacheLineSize = procInfo->Cache.LineSize;
			break;
		}



	}
	ALF_THREAD_FREE(procInfos);
	return cacheLineSize;
#elif defined(ALF_THREAD_TARGET_LINUX)
	switch (type)
	{
		case ALF_CACHE_L1D:
			return (int32_t)sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
		case ALF_CACHE_L1I:
			return (int32_t)sysconf(_SC_LEVEL1_ICACHE_LINESIZE);
		case ALF_CACHE_L2:
			return (int32_t)sysconf(_SC_LEVEL2_CACHE_LINESIZE);
		case ALF_CACHE_L3:
			return (int32_t)sysconf(_SC_LEVEL3_CACHE_LINESIZE);
		case ALF_CACHE_L4:
			return (int32_t)sysconf(_SC_LEVEL4_CACHE_LINESIZE);
		default:
			return -1;
	}
#elif defined(ALF_THREAD_TARGET_APPLE)
	return 0;
#endif
}
