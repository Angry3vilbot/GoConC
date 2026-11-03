// package main
//
// import (
//     "sync"
//     "time"
// )
//
// func eat(wg *sync.WaitGroup, fm chan struct{}, forks map[int]chan int, index int) {
//     defer wg.Done()
//
//     fm <- struct{}{}
//     println(index+1, " wants to eat.")
//     forks[index] <- 1
//     println(index+1, " took fork ", index+1)
//     forks[(index+1)%len(forks)] <- 1
//     println(index+1, " took fork ", (index+1)%len(forks)+1)
//     println(index+1, " eating...")
//
//     time.Sleep(time.Second * 5)
//
//     <-forks[index]
//     <-forks[(index+1)%len(forks)]
//     <-fm
//     println(index+1, " done!")
// }
//
// func main() {
//     waitGroup := sync.WaitGroup{}
//     numberOfThreads := 5
//     footMan := make(chan struct{}, numberOfThreads-1)
//     forks := make(map[int]chan int)
//
//     waitGroup.Add(numberOfThreads)
//     for i := 0; i < numberOfThreads; i++ {
//         forks[i] = make(chan int, 1)
//     }
//     for i := 0; i < numberOfThreads; i++ {
//         go eat(&waitGroup, footMan, forks, i)
//     }
//
//     waitGroup.Wait()
// }

#include <goconc.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct eat_args {
    gc_waitgroup* wg;
    gc_channel* fm;
    int index;
    gc_channel** forks;
    int number_of_threads;
} eat_args;
void eat(void* arg) {
    eat_args* args = arg;

    int dummy = 1;
    gc_send(args->fm, &dummy);
    printf("%d wants to eat.\n", args->index + 1);
    gc_send(args->forks[args->index], &dummy);
    printf("%d took fork %d\n", args->index + 1, args->index + 1);
    gc_send(args->forks[(args->index + 1) % args->number_of_threads], &dummy);
    printf("%d took fork %d\n", args->index + 1, (args->index + 1) % args->number_of_threads + 1);

    printf("%d eating...\n", args->index + 1);
    /* TODO: Sleep goes here once implemented
             For now, just yield so that it's not just a plain sequence */
    gc_Gosched();

    gc_receive(args->forks[args->index], &dummy);
    gc_receive(args->forks[(args->index + 1) % args->number_of_threads], &dummy);
    gc_receive(args->fm, &dummy);
    printf("%d done!\n", args->index + 1);

    gc_wg_done(args->wg);
}

int gc_user_main() {
    gc_waitgroup* wait_group = gc_wg_make();
    int numberOfThreads = 5;
    // Empty struct has a size of 0, which is not allowed.
    // TODO: Maybe allow unit size of 0? Don't think it can cause issues.
    gc_channel* foot_man = gc_make(sizeof(int), numberOfThreads - 1);
    gc_channel* forks[numberOfThreads];

    gc_wg_add(wait_group, numberOfThreads);
    for (int i = 0; i < numberOfThreads; i++) {
        forks[i] = gc_make(sizeof(int), 1);
    }
    eat_args *args = malloc(numberOfThreads * sizeof *args);
    for (int i = 0; i < numberOfThreads; i++) {
        args[i].wg = wait_group;
        args[i].fm = foot_man;
        args[i].forks = forks;
        args[i].index = i;
        args[i].number_of_threads = numberOfThreads;

        gc_go(eat, &args[i]);
    }

    gc_wg_wait(wait_group);
    // Cleanup
    gc_wg_free(wait_group);
    gc_free(foot_man);
    for (int i = 0; i < numberOfThreads; i++) {
        gc_free(forks[i]);
    }
    free(args);
    return 0;
}