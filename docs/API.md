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
> You must call [gc_free()](#gc_free()) after you finish using the channel (or at the end of the program) to free the memory allocated to the channel, otherwise you'll get a memory leak.
#### Parameters
 - **unit_size** - The size of one unit of data, e.g. <code>sizeof(int)</code>
 - **buffer_size** - The size of the buffer of a buffered channel. Use <code>0</code> for an unbuffered channel
#### Returns
 - The created `gc_channel` on success
 - `nullptr` on allocation failure
```C
gc_channel* channel = gc_make(sizeof(int), 0);  
```

### gc_free()
```C
void gc_free(gc_channel* chan)
```
Frees memory allocated to a channel and its buffer (if applicable).
> [!WARNING]
> Must be called after you finish using the channel (or at the end of the program) to free the memory allocated to the channel, otherwise you'll get a memory leak.
#### Parameters
 - **chan** - The channel to free

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

### gc_close()
```C
int gc_close(gc_channel *chan)
```
Closes the channel.
#### Parameters
 - **chan** - The pointer to the channel to close
#### Returns
 - **GC_CHANNEL_OK** on success

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
## Mutex
TODO
## WaitGroup
TODO
## Once
TODO
## Select
TODO