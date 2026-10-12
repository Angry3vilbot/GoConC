// int check_registers(context *from, context *to, uintptr_t base);
//                              rdi            rsi           rdx
//
// Loads test values into the registers of a context based on the provided base.
// Then switches the context to the other one, which does the same thing.
// Upon switching back, checks that the registers end up with correct values.

.intel_syntax noprefix

#ifdef __APPLE__
#	define SYM(name) _##name
#else
#	define SYM(name) name
#endif

.text

.p2align 4
.globl SYM(check_registers)
SYM(check_registers):
	# Save the existing registers on the stack
	push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    # Save the base
    push rdx
	# Load the test values
	mov rbx, rdx
	imul rbp, rdx, 2
	imul r12, rdx, 3
	imul r13, rdx, 4
	imul r14, rdx, 5
	imul r15, rdx, 6
	# Switch to the other context
	call SYM(ctx_switch)
	# Load the base
	pop rax
	# Check the values in the loaded registers
	mov rcx, rax
	cmp rcx, rbx
	jne Ltest_fail

	imul rcx, rax, 2
    cmp rcx, rbp
    jne Ltest_fail

    imul rcx, rax, 3
    cmp rcx, r12
    jne Ltest_fail

    imul rcx, rax, 4
    cmp rcx, r13
    jne Ltest_fail

    imul rcx, rax, 5
    cmp rcx, r14
    jne Ltest_fail

    imul rcx, rax, 6
    cmp rcx, r15
    jne Ltest_fail

    # Load the return value
    mov eax, 0
    jmp Ltest_finish

Ltest_fail:
	# Load the return value
	mov eax, 2026

Ltest_finish:
	# Restore the callee-saved registers from the stack
	pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ret

#ifdef __linux__
.section .note.GNU-stack,"",@progbits
#endif

