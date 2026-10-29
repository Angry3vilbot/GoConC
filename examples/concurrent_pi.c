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

#include "../include/goconc/channel.h"
#include "../include/goconc/scheduler.h"

struct arg_struct {
    gc_channel* chan;
    double value;
};

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

        gc_go(term, &args[k]);
    }

    double f = 3.0;
    for (int k = 0; k < n; k++) {
        double recv;
        gc_receive(channel, &recv);
        f += recv;
    }

    gc_free(channel);
    free(args);
    return f;
}

int gc_user_main(int argc, char **argv) {
    printf("pi:%f\n", 3.14159265358979323846);
    printf("Nilakantha Series:%f\n", pi(5000));

    return 0;
}
