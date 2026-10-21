// Concurrent computation of pi.
// The implementation uses the Nilakantha Series.
// func main() {
//     fmt.Println("          math.Pi:", math.Pi)
//     fmt.Println("Nilakantha Series:", pi(5000))
// }
//
// // pi launches n goroutines to compute an
// // approximation of pi.
// func pi(n int) float64 {
//     ch := make(chan float64)
//     for k := 0; k < n; k++ {
//         go term(ch, float64(k))
//     }
//     f := 3.0
//     for k := 0; k < n; k++ {
//         f += <-ch
//     }
//     return f
// }
//
// func term(ch chan float64, k float64) {
//     ch <- 4 * math.Pow(-1, k) / ((2*k + 2) * (2*k + 3) * (2*k + 4))
// }

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "channel.h"
#include "scheduler.h"

struct arg_struct {
    gc_channel* chan;
    double value;
};

struct collect_arg {
    gc_channel* chan;
    int n;
    double sum;
};

void collect(void* args) {
    struct collect_arg* arg = args;
    for (int k = 0; k < arg->n; k++) {
        double result;
        gc_receive(arg->chan, &result);
        arg->sum += result;
    }
}

void term(void* args) {
    struct arg_struct* arg = args;
    double value = 4 * pow(-1, arg->value) / ((2*arg->value + 2) * (2*arg->value + 3) * (2*arg->value + 4));
    gc_send(arg->chan, &value);
}

// pi launches n goroutines to compute an
// approximation of pi.
double pi(int n) {
    gc_channel* channel = gc_make(sizeof(double), 0);

    struct arg_struct *args = malloc(n * sizeof *args);
    for (int k = 0; k < n; k++) {
        args[k].chan = channel;
        args[k].value = k;

        go(term, &args[k]);
    }

    struct collect_arg collect_arg;
    collect_arg.chan = channel;
    collect_arg.n = n;
    collect_arg.sum = 0;
    go(collect, &collect_arg);
    run();

    double f = 3.0 + collect_arg.sum;

    gc_free(channel);
    free(args);
    return f;
}

int main() {
    printf("pi:%f\n", 3.14159265358979323846);
    printf("Nilakantha Series:%f\n", pi(5000));
}
