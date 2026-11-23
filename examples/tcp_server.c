// package main
//
// import (
//     "bufio"
//     "fmt"
//     "log"
//     "net"
//     "strings"
// )
//
// func main() {
//
//     listener, err := net.Listen("tcp", ":8090")
//     if err != nil {
//         log.Fatal("Error listening:", err)
//     }
//
//     defer listener.Close()
//
//     for {
//
//         conn, err := listener.Accept()
//         if err != nil {
//             log.Println("Error accepting conn:", err)
//             continue
//         }
//
//         go handleConnection(conn)
//     }
// }
//
// func handleConnection(conn net.Conn) {
//
//     defer conn.Close()
//
//     reader := bufio.NewReader(conn)
//     message, err := reader.ReadString('\n')
//     if err != nil {
//         log.Printf("Read error: %v", err)
//         return
//     }
//
//     ackMsg := strings.ToUpper(strings.TrimSpace(message))
//     response := fmt.Sprintf("ACK: %s\n", ackMsg)
//     _, err = conn.Write([]byte(response))
//     if err != nil {
//         log.Printf("Server write error: %v", err)
//     }
// }

#include <ctype.h>
#include <errno.h>
#include <goconc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

void handle_connection(void* args) {
    gc_fd* fd = args;
    // A defer would've been nice here...
    char buffer[1024];
    ssize_t bytes = gc_fd_read(fd, buffer, sizeof(buffer));
    if (bytes == -1) {
        fprintf(stderr, "Failed to read the message: %s\n", strerror(errno));
        gc_fd_close(fd);
        return;
    }

    char message[1029] = "ACK: ";
    for (int i = 0; i < bytes; i++) {
        message[i + 5] = toupper(buffer[i]);
    }
    fprintf(stdout, "%s", message);
    if (gc_fd_write(fd, message, bytes + 5) == -1) {
        fprintf(stderr, "Failed to write the message: %s\n", strerror(errno));
    }
    gc_fd_close(fd);
}

int gc_user_main() {
    gc_fd* listener_fd = gc_net_listen("127.0.0.1", 8090, SOMAXCONN);
    if (listener_fd == NULL) {
        fprintf(stderr, "Failed to create listener\n");
        abort();
    }

    while (true) {
        gc_fd* client_fd = gc_net_accept(listener_fd);
        if (client_fd == NULL) {
            // We ran out of space for file descriptors
            if (errno == EMFILE || errno == ENFILE) {
                gc_sleep(5 * GC_SECOND);
                continue;
            }
            fprintf(stderr, "Failed to accept connection: %s\n", strerror(errno));
            continue;
        }
        gc_go(handle_connection, client_fd);
    }

    gc_fd_close(listener_fd);
    return 0;
}

