# Threads in RSCOS
RSCOS supports thradings.

## Table of Contents
* [Threads in RSCOS](#threads-in-RSCOS)
    * [Table of Contents](#table-of-contents)
    * [Logic](#logic)
    * [List of APIs](#list-of-APIs)

## Logic
Figure 1 shows the possible states a RVCOS thread can be in as well as the transitions between them. The threading support in RVCOS is designed for potentially short run threads that can
reactivated as needed. The reactivation allows for the rerunning of a thread without the overhead of allocating resources.

![ab81db7accd9dbcb54ca858067bda11](https://user-images.githubusercontent.com/92330153/191689224-3b6ea814-5af4-4b26-b2e0-49f6fc79fcc8.png)
Figure 1. RVCOS Thread States and Transitions

1. Thread created
2. Thread activated
3. Thread selected to run
4. Thread quantum expired or higher priority thread selected
5. Thread terminated
6. Thread unblocked due to I/O, other thread, or timeout
7. Thread blocked waiting for time, I/O or another thread

## List of APIs


### RVCThreadCreate
#### Function
Creates a thread in the RVCOS.
#### Synopsys
```
#include "RVCOS.H"
typedef TThreadReturn (*TThreadEntry)(void *);

TStatus RVCThreadCreate(TThreadEntry entry, void *param, TMemorySize memsize, TThreadPriority prio, TThreadIDRef tid);
```
#### Description
RVCThreadCreate() creates a thread in the RVCOS. Once created the thread is in the created state RVCOS_THREAD_STATE_CREATED. The entry parameter specifies the function of the
thread, and param specifies the parameter that is passed to the function. The size of the threads stack is specified by memsize, and the priority is specified by prio. The thread identifier is put into
the location specified by the tid parameter.
#### Return Value
Upon successful creation of the thread RVCThreadCreate() returns RVCOS_STATUS_SUCCESS. If either entry or tid is NULL, RVCThreadCreate() returns RVCOS_STATUS_ERROR_INVALID_PARAMETER.




### RVCThreadDelete 
#### Function
Deletes a dead thread from the RVCOS.
#### Synopsys
```
#include "RVCOS.H"
TStatus RVCThreadDelete(TThreadID thread);
```
#### Description
RVCThreadDelete() deletes the dead thread specified by thread parameter from the RVCOS.

#### Return Value
Upon successful deletion of the thread from the RVCOS, RVCThreadDelete() returns
RVCOS_STATUS_SUCCESS. If the thread specified by the thread identifier thread does not
exist, RVCOS_STATUS_ERROR_INVALID_ID is returned. If the thread does exist but is not in
the RVCOS_THREAD_STATE_DEAD state, RVCOS_STATUS_ERROR_INVALID_STATE
is returned.




### RVCThreadActivate 
#### Function
Activates a newly created or dead thread in the RVCOS.
#### Synopsys 
```
#include "RVCOS.H"
TStatus RVCThreadActivate(TThreadID thread);
```
#### Description
RVCThreadActivate() activates the newly created or dead thread specified by thread parameter in
the RVCOS. After activation the thread enters the RVCOS_THREAD_STATE_READY state,
and must begin at the entry function specified.
#### Return Value
Upon successful activation of the thread in the RVCOS, RVCThreadActivate() returns
RVCOS_STATUS_SUCCESS. If the thread specified by the thread identifier thread does not
exist, RVCOS_STATUS_ERROR_INVALID_ID is returned. If the thread does exist but is not in
the newly created or dead sates, RVCOS_THREAD_STATE_CREATED or
RVCOS_THREAD_STATE_DEAD, RVCOS_STATUS_ERROR_INVALID_STATE is
returned.




### RVCThreadTerminate 
#### Function
Terminates a thread in the RVCOS.
#### Synopsys 
```
#include "RVCOS.H"
TStatus RVCThreadTerminate(TThreadID thread, TThreadReturn returnval);
```
#### Description
RVCThreadTerminate() terminates the thread specified by thread parameter in the RVCOS. After
termination the thread enters the state RVCOS_THREAD_STATE_DEAD, and the thread return
value returnval is stored for return values from RVCThreadWait(). The termination of a thread
can trigger another thread to be scheduled.
#### Return Value
Upon successful termination of the thread in the RVCOS, RVCThreadTerminate() returns
RVCOS_STATUS_SUCCESS. If the thread specified by the thread identifier thread does not
exist, RVCOS_STATUS_ERROR_INVALID_ID is returned. If the thread does exist but is in the
newly created or dead states, RVCOS_THREAD_STATE_CREATED or
RVCOS_THREAD_STATE_DEAD, RVCOS_STATUS_ERROR_INVALID_STATE is
returned.




### RVCThreadWait 
#### Function
Terminates a thread in the RVCOS.
#### Synopsys 
```
#include "RVCOS.H"
TStatus RVCThreadWait(TThreadID thread, TThreadReturnRef returnref);
```
#### Description
RVCThreadWait() waits for the thread specified by thread parameter to terminate. The return
value passed with the associated RVCThreadTerminate() call will be placed in the location
specified by returnref. RVCThreadWait() can be called multiple times per thread.
#### Return Value
Upon successful termination of the thread in the RVCOS, RVCThreadWait() returns
RVCOS_STATUS_SUCCESS. If the thread specified by the thread identifier thread does not
exist, RVCOS_STATUS_ERROR_INVALID_ID is returned. If the parameter returnref is NULL,
RVCOS_STATUS_ERROR_INVALID_PARAMETER is returned.




### RVCThreadID 
#### Function
Retrieves thread identifier of the current operating thread.
#### Synopsys 
```
#include "RVCOS.H"
TStatus RVCThreadID(TThreadIDRef threadref);
```
#### Description
RVCThreadID() puts the thread identifier of the currently running thread in the location specified
by threadref.
#### Return Value
Upon successful retrieval of the thread identifier from the RVCOS, RVCThreadID() returns
RVCOS_STATUS_SUCCESS. If the parameter threadref is NULL,
RVCOS_STATUS_ERROR_INVALID_PARAMETER is returned.




### RVCThreadState 
#### Function
Retrieves the state of a thread in the RVCOS.
#### Synopsys 
```
#include "RVCOS.H"
#define RVCOS_THREAD_STATE_CREATED ((TThreadState)0x01)
#define RVCOS_THREAD_STATE_DEAD ((TThreadState)0x02)
#define RVCOS_THREAD_STATE_RUNNING ((TThreadState)0x03)
#define RVCOS_THREAD_STATE_READY ((TThreadState)0x04)
#define RVCOS_THREAD_STATE_WAITING ((TThreadState)0x05)
TStatus RVCThreadState(TThreadID thread, TThreadStateRef state);
```
#### Description
RVCThreadState() retrieves the state of the thread specified by thread and places the state in the
location specified by state.
#### Return Value
Upon successful retrieval of the thread state from the RVCOS, RVCThreadState() returns
RVCOS_STATUS_SUCCESS. If the thread specified by the thread identifier thread does not
exist, RVCOS_STATUS_ERROR_INVALID_ID is returned. If the parameter stateref is NULL,
RVCOS_STATUS_ERROR_INVALID_PARAMETER is returned.




### RVCThreadSleep 
#### Function
Puts the current thread in the RVCOS to sleep.
#### Synopsys 
```
#include "RVCOS.H"
#define RVCOS_TIMEOUT_INFINITE ((TTick)0)
#define RVCOS_TIMEOUT_IMMEDIATE ((TTick)-1)
TStatus RVCThreadSleep(TTick tick);
```
#### Description
RVCThreadSleep() puts the currently running thread to sleep for tick ticks. If tick is specified as
RVCOS_TIMEOUT_IMMEDIATE the current process yields the remainder of its processing
quantum to the next ready process of equal priority.
#### Return Value
Upon successful sleep of the currently running thread, RVCThreadSleep() returns
RVCOS_STATUS_SUCCESS. If the sleep duration tick specified is
RVCOS_TIMEOUT_INFINITE, RVCOS_STATUS_ERROR_INVALID_PARAMETER is
returned.
