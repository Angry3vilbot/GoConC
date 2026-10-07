; void ctx_switch(context *from, context *to);
;
; Save the current context's registers on the current context's stack at *from
; and then load the new context's registers from its stack from *to
.code
PUBLIC ctx_switch

ctx_switch PROC
	; Save the current context's registers

	; Load the new context's registers
	ret
ctx_switch ENDP

end