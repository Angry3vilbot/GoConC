// package main
//
// import (
//     "fmt"
//     "time"
// )
//
// func worker(id int, jobs <-chan int, results chan<- int) {
//     for j := range jobs {
//         fmt.Println("worker", id, "started  job", j)
//         time.Sleep(time.Second)
//         fmt.Println("worker", id, "finished job", j)
//         results <- j * 2
//     }
// }
//
// func main() {
//
//     const numJobs = 5
//     jobs := make(chan int, numJobs)
//     results := make(chan int, numJobs)
//
//     for w := 1; w <= 3; w++ {
//         go worker(w, jobs, results)
//     }
//
//     for j := 1; j <= numJobs; j++ {
//         jobs <- j
//     }
//     close(jobs)
//
//     for a := 1; a <= numJobs; a++ {
//         <-results
//     }
// }

#include <stdio.h>
#include <stdlib.h>

#include "goconc.h"

typedef struct worker_args {
    int id;
    gc_channel* jobs;
    gc_channel* results;
} worker_args;
void worker(void* args) {
    worker_args* wargs = args;
    int job;
    int result;
    // Or != GC_CHANNEL_CLOSED
    while (gc_receive(wargs->jobs, &job) == GC_CHANNEL_OK) {
        printf("worker %d started job %d\n", wargs->id, job);
        gc_sleep(GC_SECOND);
        printf("worker %d finished job %d\n", wargs->id, job);
        result = job * 2;
        gc_send(wargs->results, &result);
    }
}

int gc_user_main() {
    const int num_jobs = 5;
    gc_channel* jobs = gc_make(sizeof(int), num_jobs);
    gc_channel* results = gc_make(sizeof(int), num_jobs);

    worker_args* args = calloc(3, sizeof(*args));
    for (int w = 1; w <= 3; w++) {
        args[w-1].id = w;
        args[w-1].jobs = jobs;
        args[w-1].results = results;
        gc_go(worker, &args[w-1]);
    }

    // A little extra
    gc_mono_time start = gc_monotonic_time_now();

    int* jobs_i = calloc(num_jobs, sizeof(*jobs_i));
    for (int j = 1; j <= num_jobs; j++) {
        jobs_i[j-1] = j;
        gc_send(jobs, &jobs_i[j-1]);
    }
    gc_close(jobs);

    for (int j = 1; j <= num_jobs; j++) {
        int discard;
        gc_receive(results, &discard);
    }

    gc_mono_time end = gc_monotonic_time_now();
    printf("%d jobs done in %ld seconds\n", num_jobs, gc_wall_unix(end - start));

    // Cleanup
    gc_free(jobs);
    gc_free(results);
    free(jobs_i);
    free(args);
    return 0;
}