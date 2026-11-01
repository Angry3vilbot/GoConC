# GoConC API
This is the documentation for GoConC, a Modern C library for Go-style concurrency.
## Table of Contents
- [Goroutines And The Scheduler](#goroutines-and-the-scheduler)
- [The Main Goroutine](#the-main-goroutine)
- [Channel](#channel)
- [Mutex](#mutex)
- [WaitGroup](#waitgroup)
- [Once](#once)
- [Select](#select)

## Goroutines And The Scheduler
Goroutines are created out of functions with the following signature:
```C
void function_name(void* arg)
```

### Start the scheduler loop.
#### gc_run()
```C
void gc_run()
```
Runs the scheduler loop until the run queue is emptied.
If the run queue is empty while the goroutine count is above 0, the scheduler considers it a deadlock
and will abort execution of the program.
```C
void print(void* args) {
    printf("I am a goroutine\n");
}

int main() {
    gc_go(print, nullptr);
    gc_go(print, nullptr);
    gc_run();

    return 0;
}
```
### Start a new goroutine
#### gc_go()
```C
int gc_go(entry_function function, void *arg)
```
Starts a new goroutine with GC_DEFAULT_STACK_SIZE (64 KB) of stack size.
#### Parameters
 - **function** - the function that runs inside the goroutine
 - **arg** - the argument for said function
#### Returns
 - **GC_SUCCESS** on success
 - **GC_GOROUTINE_ALLOC_ERROR** if the goroutine could not be allocated
 - **GC_STACK_ALLOC_ERROR** if the stack could not be allocated
<hr>

#### gc_go_sized()
```C
int gc_go_sized(entry_function function, void *arg, size_t size)
```
Starts a new goroutine with the specified stack size.
#### Parameters
 - **function** - the function that runs inside the goroutine
 - **arg** - the argument for said function
 - **size** - the stack size to allocate
#### Returns
 - **GC_SUCCESS** on success
 - **GC_GOROUTINE_ALLOC_ERROR** if the goroutine could not be allocated
 - **GC_STACK_ALLOC_ERROR** if the stack could not be allocated
<hr>

#### gc_Gosched()
```C
void gc_Gosched()
```
Pauses the currently running goroutine and yields control to the scheduler.
```C
void print_A(void* args) {
    printf("A0 ");
    gc_Gosched();
    printf("A1 ");
    gc_Gosched();
    printf("A2 ");
}

void print_B(void* args) {
    printf("B0 ");
    gc_Gosched();
    printf("B1 ");
    gc_Gosched();
    printf("B2 ");
}

int main() {
    gc_go(print_A, nullptr);
    gc_go(print_B, nullptr);
    
    gc_run();

    return 0;
}
```
Output: `A0 B0 A1 B1 A2 B2 `
## The Main Goroutine
In Go, the main function runs on its own goroutine. This is what allows you to do things such as e.g. yield control to the scheduler, or get blocked by channel operations or a mutex, without the rest of the goroutines being blocked along with it. In GoConC, this can be achieved in one of two ways.
### Custom Main Goroutine Using gc_main()
You can create your own main goroutine by calling `gc_main()`
```C
int gc_main(int (*func)(void *), void *arg)
```
#### Parameters
 - **func** - The function to run, has to have the following signature:
    ```C
    int name(void* arg)
    ```
 - **arg** - The function's argument
#### Returns
 - The return value of the function (the exit code)

`gc_main()` will pass the argument to the function, as well as start the scheduler loop.
### gc_user_main() And The GoConC::main Sublibrary
Alternatively, you can link the GoConC::main sublibrary and define the `int gc_user_main()` function as your main goroutine. The sublibrary provides its own `main()` function that makes `gc_user_main()` the main goroutine using `gc_main()`, passes it command line arguments (if needed), and starts the scheduler loop.

## Channel
### gc_make()
```C
gc_channel* gc_make(size_t unit_size, int buffer_size)
```
Creates a new channel and allocates memory for it and its buffer (if applicable).
> [!WARNING]
> You must call [gc_free()](#gc_free) after you finish using the channel (or at the end of the program) to free the memory allocated to the channel, otherwise you'll get a memory leak.
#### Parameters
 - **unit_size** - The size of one unit of data, e.g. <code>sizeof(int)</code>
 - **buffer_size** - The size of the buffer of a buffered channel. Use <code>0</code> for an unbuffered channel
#### Returns
 - The created `gc_channel` on success
 - `nullptr` on allocation failure
```C
gc_channel* channel = gc_make(sizeof(int), 0);  
```
<hr/>

### gc_free()
```C
void gc_free(gc_channel* chan)
```
Frees memory allocated to a channel and its buffer (if applicable).
> [!WARNING]
> Must be called after you finish using the channel (or at the end of the program) to free the memory allocated to the channel, otherwise you'll get a memory leak.
#### Parameters
 - **chan** - The channel to free
<hr/>

### gc_send()
```C
int gc_send(gc_channel *chan, void *data)
```
Sends data into the channel
Uses direct handoff when possible instead of storing the data in the buffer. Unbuffered channels only use direct handoff.
#### Parameters
 - **chan** - The channel to send data into
 - **data** - The data to send into the channel
#### Returns
 - **GC_CHANNEL_OK** on success
 - **GC_CHANNEL_CLOSED** if the channel was closed
```C
gc_channel* channel = gc_make(sizeof(int), 10);
...
int data = 278;
gc_send(channel, &data); 
```
<hr/>

### gc_receive()
```C
int gc_receive(gc_channel *chan, void *data)
```
Receives data from the channel.
#### Parameters
 - **chan** - The channel to receive data from
 - **data** - The pointer to where to place the data
#### Returns
 - **GC_CHANNEL_OK** on success
 - **GC_CHANNEL_CLOSED** if the channel was closed
```C
int received_value;
if (gc_receive(channel, &received_value) == GC_CHANNEL_OK) {
    printf("Receiver has received a value %d\n", received_value);
}
```
<hr/>

### gc_close()
```C
int gc_close(gc_channel *chan)
```
Closes the channel.
#### Parameters
 - **chan** - The pointer to the channel to close
#### Returns
 - **GC_CHANNEL_OK** on success
<hr/>

### gc_try_receive()
```C
int gc_try_receive(gc_channel *chan, void *data)
```
Receive data from the channel without blocking.
#### Parameters
 - **chan** - The channel to receive data from
 - **data** - The pointer to where to place the data
#### Returns
 - **GC_CHANNEL_OK** on success
 - **GC_CHANNEL_CLOSED** if the channel was closed
 - **GC_CHANNEL_WOULD_BLOCK** if the receive operation would block
<hr/>

### gc_try_send()
```C
int gc_try_send(gc_channel *chan, void *data)
```
Send data into the channel without blocking.
#### Parameters
 - **chan** - The channel to send data into
 - **data** - The data to send into the channel
#### Returns
 - **GC_CHANNEL_OK** on success
 - **GC_CHANNEL_CLOSED** if the channel was closed
 - **GC_CHANNEL_WOULD_BLOCK** if the send operation would block
<br><br>

## Mutex
> [!WARNING]
> Do NOT copy/pass by value, it will corrupt the wait queue.
### gc_mutex_lock()
```C
void gc_mutex_lock(gc_mutex* mu)
```
Tries to lock the Mutex. If it is already locked, the goroutine is blocked until it becomes available.
#### Parameters
 - **mu** - The Mutex to lock
<hr/>

### gc_mutex_unlock()
```C
void gc_mutex_unlock(gc_mutex* mu)
```
Unlocks a locked Mutex. Aborts if the Mutex is already unlocked.
> [!NOTE]
> A locked Mutex can be unlocked by a different goroutine than the one that locked it.
#### Parameters
 - **mu** - The Mutex to unlock
<hr/>

### gc_mutex_trylock()
```C
bool gc_mutex_trylock(gc_mutex* mu)
```
Try to lock a Mutex Lock and report the result
> [!NOTE]
> There are very few cases when one would legitimately need to use this.
#### Parameters
 - **mu** - The Mutex to try locking
#### Returns
 - `true` on success
 - `false` if it would be blocked
<br><br>

## WaitGroup
### gc_wg_make()
```C
gc_waitgroup* gc_wg_make()
```
Creates a new WaitGroup and initializes its queue.
> [!WARNING]
> You must call [gc_wg_free](#gc_wg_free) after you finish using the WaitGroup
> (or at the end of the program) to free the memory allocated to it, otherwise you'll get a memory leak.
#### Returns
- The pointer to the created gc_waitgroup on success
- `nullptr` on allocation failure
<hr/>

### gc_wg_free()
```C
void gc_wg_free(gc_waitgroup* wg)
```
Frees memory allocated to a WaitGroup.
> [!WARNING]
> Must be called after you finish using the WaitGroup (or at the end of the program)
> to free the memory allocated to it, otherwise you'll get a memory leak.
#### Parameters
 - **wg** - The WaitGroup to free
<hr/>

### gc_wg_add()
```C
void gc_wg_add(gc_waitgroup* wg, long delta)
```
Add the delta to the WaitGroup's counter.
> [!NOTE]
> If the counter becomes zero, all goroutines blocked by [gc_wg_wait](#gc_wg_wait) are unblocked.
> If the counter goes negative, the program aborts.
#### Parameters
 - **wg** - The target WaitGroup
 - **delta** - The value to add to the WaitGroup's counter
<hr/>

### gc_wg_done()
```C
void gc_wg_done(gc_waitgroup* wg)
```
Decrement the WaitGroup's counter by 1. Equivalent to calling <code>gc_wg_add(wg, -1)</code>
#### Parameters
 - **wg** - The target WaitGroup
<hr/>

### gc_wg_wait()
```C
void gc_wg_wait(gc_waitgroup* wg)
```
Wait until the WaitGroup's task counter is zero.
Parks the goroutine until the counter reaches zero.
#### Parameters
 - **wg** - The target WaitGroup
<br><br>

## Once
Makes sure that the given function only ever runs once
> [!WARNING]
> Do NOT copy/pass by value, it will corrupt the wait queue.

### gc_once_do()
```C
void gc_once_do(gc_once* once, entry_function func, void *arg)
```
Calls `func` if and only if [gc_once_do](#gc_once_do) is being called for the first time with the given instance of `once`. All other calls made with the
same instance will return without calling their `func`.
> [!WARNING]
> If `func` calls [gc_once_do](#gc_once_do), you will get a deadlock.
#### Parameters
 - **once** - The pointer to the shared Once
 - **func** - The function to run only once
 - **arg** - The argument for the function
<br><br>

## Select
### gc_select_case
```C
typedef struct gc_select_case {
    gc_channel *channel;
    GC_CASE_DIRECTION direction;
    void* data;
    bool closed;
} gc_select_case;
```
A case of a select statement.
This struct represents an individual case of a select. One or more of these can be placed in an array and provided to [gc_select](#gc_select) to be considered part of one select statement.
#### Fields
   - **channel** - The channel to send/receive data from
   - **direction** - The direction of the operation, either `GC_CASE_SEND` or `GC_CASE_RECEIVE`
   - **data** - The data to send or the pointer to where to place the received data
   - **closed** - Whether the case was chosen because its channel got closed. Set to `false` by default
<hr/>

### gc_select()
```C
int gc_select(gc_select_case* case_list, size_t num_cases, bool has_default)
```
Wait on multiple channels.
Lets a goroutine wait on multiple communication operations.
Blocks until one of its cases can run, then it executes that case.
Checks cases in random order for fairness.
> [!WARNING]
> The maximum number of cases in a select is capped at 64
#### Parameters
 - **case_list** - The array of [gc_select_case](#gc_select_case) cases
 - **num_cases** - The amount of cases in the list
 - **has_default** - Whether the select operates as if it has a default case, i.e. does not block if no case can run
#### Returns
 - The index of the winning case, `-1` if no case is available and `has_default == true`
<br><br>