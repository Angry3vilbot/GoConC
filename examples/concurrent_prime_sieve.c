// // A concurrent prime sieve
//
// package main
//
// import "fmt"
//
// // Send the sequence 2, 3, 4, ... to channel 'ch'.
// func Generate(ch chan<- int) {
//     for i := 2; ; i++ {
//         ch <- i // Send 'i' to channel 'ch'.
//     }
// }
//
// // Copy the values from channel 'in' to channel 'out',
// // removing those divisible by 'prime'.
// func Filter(in <-chan int, out chan<- int, prime int) {
//     for {
//         i := <-in // Receive value from 'in'.
//         if i%prime != 0 {
//             out <- i // Send 'i' to 'out'.
//         }
//     }
// }
//
// // The prime sieve: Daisy-chain Filter processes.
// func main() {
//     ch := make(chan int) // Create a new channel.
//     go Generate(ch)      // Launch Generate goroutine.
//     for i := 0; i < 10; i++ {
//         prime := <-ch
//         fmt.Println(prime)
//         ch1 := make(chan int)
//         go Filter(ch, ch1, prime)
//         ch = ch1
//     }
// }

#include <stdio.h>
#include <goconc.h>

struct generate_args {
    gc_channel* channel;
    gc_waitgroup* waitgroup;
};

// Send the sequence 2, 3, 4, ... to channel 'ch'.
void Generate(void* args) {
    struct generate_args* args_ = args;

    for (int i = 2; ; i++) {
        if (gc_send(args_->channel, &i) != GC_CHANNEL_OK) {
            break;
        }
    }

    gc_wg_done(args_->waitgroup);
}

// Copy the values from channel 'in' to channel 'out',
// removing those divisible by 'prime'.
struct filter_args {
    gc_channel* in;
    gc_channel* out;
    gc_waitgroup* waitgroup;
    int prime;
};

void Filter(void* args) {
    struct filter_args* args_ = args;

    while (true) {
        int value = 0;
        if (gc_receive(args_->in, &value) != GC_CHANNEL_OK) {
            // Propagate closure downstream. Channel ownership remains with
            // gc_user_main until all goroutines have exited.
            if (args_->out != nullptr) {
                gc_close(args_->out);
            }
            gc_wg_done(args_->waitgroup);
            return;
        }

        if (args_->out != nullptr) {
            if (value % args_->prime != 0 && gc_send(args_->out, &value) == GC_CHANNEL_CLOSED) {
                gc_close(args_->in);
                gc_wg_done(args_->waitgroup);
                return;
            }
        }
    }
}

// The prime sieve: Daisy-chain Filter processes.
int gc_user_main() {
    // A WaitGroup is required because, unlike Go, we cannot abandon the other goroutines after returning
    // from main, nor do we have a dedicated garbage collector.
    // The process of finishing execution is as follows:
    // 1. Once the last Filter goroutine has been launched, the main goroutine closes the input channel
    // of the last Filter.
    // 2. The last Filter will receive a null channel for output, this will tell it to discard all values
    // it receives from Generate. It will instead simply call gc_wg_done and return.
    // 3. Because the second to last filter's output got closed, it will proceed to close its input channel,
    // call gc_wg_done and return.
    // 4. This process will continue until the Generate goroutine sees a closed channel, at which point
    // it will call gc_wg_done and return, allowing the main goroutine to continue to the return statement.
    gc_waitgroup* waitgroup = gc_wg_make();
    gc_channel* ch = gc_make(sizeof(int), 0);

    struct generate_args generate_args = {
        .channel = ch,
        .waitgroup = waitgroup
    };

    gc_wg_add(waitgroup, 11); // Generate plus ten filters.
    gc_go(Generate, &generate_args);

    struct filter_args args[10];

    for (int i = 0; i < 10; i++) {
        int prime = 0;
        gc_receive(ch, &prime);
        printf("%d\n", prime);

        gc_channel* ch1 = nullptr;
        if (i < 9) {
            ch1 = gc_make(sizeof(int), 0);
        }

        args[i] = (struct filter_args) {
            .in = ch,
            .out = ch1,
            .waitgroup = waitgroup,
            .prime = prime
        };
        gc_go(Filter, &args[i]);

        if (i == 9) {
            // This stops Generate and begins closure propagation through the filter chain.
            gc_close(ch);
        }
        ch = ch1;
    }
    // Keep generate_args and args alive until every goroutine is finished.
    gc_wg_wait(waitgroup);
    // Cleanup
    // Every input channel is args[i].in; the final output is args[9].out, which is null.
    for (int i = 0; i < 10; i++) {
        gc_free(args[i].in);
    }
    gc_wg_free(waitgroup);

    return 0;
}