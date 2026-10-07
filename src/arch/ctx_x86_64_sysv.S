// void ctx_switch(context *from, context *to);
//
// Save the current context's registers on the current context's stack through *from
// and then load the new context's registers from its stack from *to

.intel_syntax noprefix

#ifdef __APPLE__
#	define SYM(name) _##name
#else
#	define SYM(name) name
#endif

.text

.p2align 4
.globl SYM(ctx_switch)
SYM(ctx_switch):
	# Save the current context's registers
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15
	# Store the stack pointer of the current context
	mov [rdi], rsp
	# Load the new context's stack pointer
	mov rsp, [rsi]
	# Load the new context's registers (reverse order)
	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx
	# Return
	ret

.p2align 4
.globl SYM(ctx_trampoline)
SYM(ctx_trampoline):
	mov rdi, r13			# Load the entry function's argument from the forged frame
	sub rsp, 8				# Align the stack pointer to 0 % 16 (currently at 8 % 16)
	call r12				# Call the entry function
	# NOTE: For later, if on ELF I get an error about relocation in a PIE build,
	# do `call SYM(goroutine_exit)@PLT` only on Linux
	call SYM(goroutine_exit)
	ud2						# goroutine_exit must never return back here

#ifdef __linux__
.section .note.GNU-stack,"",@progbits
#endif