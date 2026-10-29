# GoConC
A library for modern C providing Go-style concurrency features
## Features
- Goroutines: starting, including using a custom stack size, and yielding.
- Channels: buffered and unbuffered, `close()`.
- Mutex Locks: `lock()`, `unlock()`, `trylock()` *if you really need it*.
- WaitGroups: `Add()`, `Done()` and `Wait()`.
- Gosched: pause a running goroutine and return back to the scheduler
- Once: run a given function only once, similar to `sync.Once`.
- gc_main: run a given function as the main goroutine.
- GoConC::main: A sublibrary for running gc_main with less code using `gc_user_main`.
- Select: wait on multiple channel operations at the same time.
- Backend required for managing goroutines, including:
  - A fast context switch written in assembly
  - A scheduler with deadlock detection
- Fully tested using CTest under sanitizers
## Installation and Usage
### Option 1: FetchContent
```cmake
include(FetchContent)

FetchContent_Declare(GoConC
    GIT_REPOSITORY https://github.com/Angry3vilbot/GoConC.git
    GIT_TAG v0.1.0
)

FetchContent_MakeAvailable(GoConC)

target_link_libraries(app PRIVATE GoConC::GoConC)
# If you need the sublibrary
target_link_libraries(app PRIVATE GoConC::main)
```
### Option 2: Download the source, install locally and use find_package
```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DGOCONC_BUILD_TESTS=OFF
cmake --build build
cmake --install build --prefix /usr/local
```
```cmake
find_package(GoConC 0.1.0 REQUIRED)
target_link_libraries(app PRIVATE GoConC::GoConC)
# If you need the sublibrary
target_link_libraries(app PRIVATE GoConC::main)
```
## Examples
### 1. Goroutines
If you are using a custom `main` function that isn't a goroutine, you can start the goroutine scheduler by using `gc_run()`. Once all goroutines finish executing, the execution will return back to `main`.
```C
#include <stdio.h>
#include <goconc.h>

void print_A(void* arg) {
    printf("I am goroutine A!\n");
}


int main() {
    gc_go(print_A, nullptr);
    gc_run();

    return 0;
}
```
### 2. Using the main goroutine
You can instead create a main goroutine that will start the scheduler on its own. There's two ways of accomplishing this: calling `gc_main()` directly, or using the `GoConC::main` sublibrary.
#### gc_main
```C
#include <stdio.h>
#include <goconc.h>

void goroutine(void* arg) {
    printf("I am a goroutine.\n");
}

int actual_main(void* arg) {
    printf("I am the main goroutine.\n");
    gc_go(goroutine, nullptr);
    
    return 0;
}

int main() {
    int retv = gc_main(actual_main, nullptr);
    // Any code you wish to run outside of the main goroutine goes here
    return retv;
}
```
#### GoConC::main
```C
#include <stdio.h>
#include <goconc.h>

void goroutine(void* arg) {
    printf("I am a goroutine.\n");
}

int gc_user_main(int argc, char **argv) {
    printf("I am the main goroutine.\n");
    gc_go(goroutine, nullptr);
    
    return 0;
}
```
### 3. Arguments for goroutines
`gc_go()`, besides the function, accepts a single argument. This argument can be either a pointer to a value (one argument), or a pointer to an argument struct (multiple arguments). It is then provided to the goroutine's entry function for use.
```C
#include <stdio.h>

#include <goconc.h>

void goroutine(void* arg) {
    int* val = arg;
    printf("%d\n", *val);
}

struct args {
    int a;
    int b;
    int c;
};
void multi_arg_goroutine(void* arg) {
    struct args* args = arg;
    printf("A: %d\n", args->a);
    printf("B: %d\n", args->b);
    printf("C: %d\n", args->c);
}

int main(int argc, char **argv) {
    int num = 924;
    gc_go(goroutine, &num);
    gc_run();

    struct args args = {.a = 10, .b = 20, .c = 30};
    gc_go(multi_arg_goroutine, &args);
    gc_run();

    return 0;
}
```
### 4. Channels
Channels are a way for goroutines to pass values between one another. They work similarly to how Go's channels work. Note however, that you need to pass a pointer to data in order to send/receive data.
```C
#include <stdio.h>
#include <stdlib.h>

#include "goconc/channel.h"
#include "goconc/scheduler.h"

void producer(void* arg) {
    gc_channel* channel = arg;
    int* data = malloc(20 * sizeof(int));
    for (int i = 0; i < 20; i++) {
        data[i] = rand();
        gc_send(channel, &data[i]);
        printf("Producer has sent a value %d\n", data[i]);
    }
    free(data);
    gc_close(channel);
}

void consumer(void* arg) {
    gc_channel* channel = arg;
    int received_value;
    while (gc_receive(channel, &received_value) == GC_CHANNEL_OK) {
        printf("Consumer received value %d\n", received_value);
        gc_Gosched();
    }
}

int main() {
    gc_channel* channel = gc_make(sizeof(int), 10);
    gc_go(producer, channel);
    gc_go(consumer, channel);
    gc_run();
    // IMPORTANT!
    gc_free(channel);
    return 0;
}
```
### 5. Yield to the scheduler
Just like in Go, you can use `gc_Gosched()` to pause the current goroutine and give the control back to the scheduler.
```C
#include <stdio.h>
#include <goconc.h>

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
void print_C(void* args) {
    printf("C0 ");
    gc_Gosched();
    printf("C1 ");
    gc_Gosched();
    printf("C2 ");
}

int gc_user_main(int argc, char **argv) {
    gc_go(print_A, nullptr);
    gc_go(print_B, nullptr);
    gc_go(print_C, nullptr);

    return 0;
}
```
### 6. Once
You can create an instance of `gc_once`, and then use `gc_once_do` to run a given function only once. Only the first call to `gc_once_do` will execute the provided function.
```C
#include "goconc/once.h"
#include "goconc/scheduler.h"

int test_value = 0;

void init(void* args) {
    test_value++;
    gc_Gosched();
    test_value++;
}

void doer(void* args) {
    gc_once* once = args;
    gc_once_do(once, init, nullptr);
}

int main() {
    gc_once once = GC_ONCE_INIT; // Or {{0}}

    for (int i = 0; i < 10; i++) {
        gc_go(doer, &once);
    }
    gc_run();
    // test_value == 2
    return 0;
}
```
### 7. Select
The select statement in Go is essentially a switch statement, but for channel operations. In GoConC, `gc_select` returns an index of the winning case, or -1 if none were available and `has_default` is `true`
```C
#include <stdio.h>
#include "goconc.h"

void selector(void* arg) {
    gc_channel* channel = arg;
    int recval = 0;
    int sendval = 458;
    // Duplicate channels with opposing operations in a gc_select are ignored
    gc_select_case cases[] = {
        {
            .channel = channel,
            .direction = GC_CASE_RECEIVE,
            .data = &recval,
            .closed = false
        },
        {
            .channel = channel,
            .direction = GC_CASE_SEND,
            .data = &sendval,
            .closed = false
        }
    };
    int index = gc_select(cases, 2, true);
    switch (index) {
        case 0:
            printf("Received from channel 1: %d\n", recval);
            break;
        case 1:
            printf("Sent to channel 1\n");
            break;
        default:
            printf("Default case\n");
    }
    printf("I am not blocked!\n");
}

int main() {
    gc_channel* chan = gc_make(sizeof(int), 0);

    gc_go(selector, chan);
    gc_run();

    gc_free(chan);
    return 0;
}
```
### More Examples
If you want to see more examples, you can look at the tests inside the `tests` directory, or look at the `examples` folder.
## Compatibility
As of version 0.1.0, the library is `ONLY COMPATIBLE WITH THE x86-64 System V ARCHITECTURE`. This is due to the fact that each architecture requires its own custom implementation of the context switch, trampoline and goroutine context initialization.
## Limitations
As of version 0.1.0:
- The library provides only single-threaded concurrency. All of its features are NOT guaranteed to be thread-safe (assume that none of them are).
- Arguments provided to goroutines must be pointers, and they MUST outlive the goroutine they're provided to (or at least until they're no longer needed). This is because the arguments live in the `gc_go`'s caller.
- Channels and WaitGroups must be destroyed manually, otherwise you get a memory leak. Make sure that you only destroy them once you're done using them.
- Mutex Locks (or anything that uses them) MUST NOT be passed by value or copied, because the copy will have a corrupted wait queue.
- The scheduler is fairly basic, it is single-threaded, and it does not have preemption.
- Goroutines have a fixed stack size. By default, i.e. when calling `gc_go()`, their stack size is 64 KB. This means that, theoretically, you could end up running out of memory, both in terms of total system memory, and goroutine's personal stack (especially if you're doing deep recursion). You can use `gc_go_sized` to set a custom stack size.
- Blocking calls, such as `sleep()` or `read()` block the entire thread and, as such, every goroutine.