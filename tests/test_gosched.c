// Copyright (c) 2026 Mykhailo Balaker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>

#include "goconc/scheduler.h"
// Test that yielding with gc_Gosched works as intended
// Expected output: A0 B0 C0 A1 B1 C1 A2 B2 C2

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

int main() {
    gc_go(print_A, nullptr);
    gc_go(print_B, nullptr);
    gc_go(print_C, nullptr);
    gc_run();

    return 0;
}