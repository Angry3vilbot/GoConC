// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func main() {
//
//     requests := make(chan int, 5)
//     for i := 1; i <= 5; i++ {
//         requests <- i
//     }
//     close(requests)
//
//     limiter := time.Tick(200 * time.Millisecond)
//
//     for req := range requests {
//         <-limiter
//         fmt.Println("request", req, time.Now())
//     }
//
//     burstyLimiter := make(chan time.Time, 3)
//
//     for range 3 {
//         burstyLimiter <- time.Now()
//     }
//
//     go func() {
//         for t := range time.Tick(200 * time.Millisecond) {
//             burstyLimiter <- t
//         }
//     }()
//
//     burstyRequests := make(chan int, 5)
//     for i := 1; i <= 5; i++ {
//         burstyRequests <- i
//     }
//     close(burstyRequests)
//     for req := range burstyRequests {
//         <-burstyLimiter
//         fmt.Println("request", req, time.Now())
//     }
// }

#include <stdio.h>
#include <stdlib.h>

#include "goconc.h"

typedef struct helper_args {
    gc_channel* bursty_limiter;
    gc_timer* helper_timer;
    gc_waitgroup* waitgroup;
} helper_args;
void helper(void* args) {
    helper_args* hargs = args;

    gc_wall_time* time;
    while (gc_receive(gc_timer_get_channel(hargs->helper_timer), &time) == GC_CHANNEL_OK) {
       gc_send(hargs->bursty_limiter, &time);
    }

    gc_wg_done(hargs->waitgroup);
}

int gc_user_main() {
    gc_channel* requests = gc_make(sizeof(int), 5);

    int reqs[5] = {1, 2, 3, 4, 5};
    for (int i = 1; i <= 5; i++) {
        gc_send(requests, &reqs[i-1]);
    }
    gc_close(requests);

    gc_timer* limiter = gc_ticker_make(200 * GC_MILLISECOND);

    int req;
    char buffer[64];
    while (gc_receive(requests, &req) == GC_CHANNEL_OK) {
        gc_wall_format(gc_wall_clock_time_now(), buffer, 64, "%m-%d-%Y %H:%M:%S", false, true);
        printf("request %d %s\n", req, buffer);
    }
    printf("\n");

    gc_channel* bursty_limiter = gc_make(sizeof(gc_wall_time), 3);
    // Needed here to be able to stop it and the goroutines
    gc_timer* helper_timer = gc_ticker_make(200 * GC_MILLISECOND);
    // Needed to allow the helpers to all stop before the cleanup
    gc_waitgroup* wg = gc_wg_make();
    helper_args* hargs = calloc(3, sizeof(*hargs));
    for (int i = 0; i <= 2; i++) {
        hargs[i].helper_timer = helper_timer;
        hargs[i].bursty_limiter = bursty_limiter;
        hargs[i].waitgroup = wg;
        gc_go(helper, &hargs[i]);
        gc_wg_add(wg, 1);
    }

    gc_channel* bursty_requests = gc_make(sizeof(int), 5);
    for (int i = 1; i <= 5; i++) {
        gc_send(bursty_requests, &reqs[i-1]);
    }
    gc_close(bursty_requests);
    for (int i = 1; i <= 5; i++) {
        int discard;
        gc_receive(bursty_requests, &discard);
        gc_wall_format(gc_wall_clock_time_now(), buffer, 64, "%m-%d-%Y %H:%M:%S", false, true);
        printf("request %d %s\n", discard, buffer);
    }
    // Close the helper ticker's channel to notify the helpers that they should stop
    gc_ticker_close(helper_timer);
    // Wait until all of the helpers acknowledge the ticker getting closed
    gc_wg_wait(wg);

    // Cleanup
    gc_wg_free(wg);
    gc_ticker_free(helper_timer);
    gc_ticker_free(limiter);
    gc_free(bursty_limiter);
    gc_free(bursty_requests);
    gc_free(requests);
    free(hargs);

    return 0;
}