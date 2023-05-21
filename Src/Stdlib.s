section .text
global OutputNumber10
global InputNumber10 

;-------------------------------------------
;Put number in dec form in stdout
;-------------------------------------------
;EXPECTS:	None
;
;ENTRY:		rdi - number for output
;
;OUTPUT:	None
;
;DESTROYS:	rax, rbx, rcx, rdx, r11, rsi, r8
;-------------------------------------------
OutputNumber10:
    mov rax, rdi

    xor r8, r8
    
	mov r11, 10
    mov rbx, MAX_SYMBOL_IN_NUMBER
	
	mov rcx, rax
	shr rcx, 63
	jz .next				; if (rax < 0)
		xor rax, -1			; 
		inc rax				;rax *= -1

		mov byte Buffer[r8], '-'
		inc r8

		jmp .next

	.next:
		xor rdx, rdx		;rdx = 0
		div r11				;rax = rdxrax / 10 
							;rdx = rax % 10

		add dl, '0'			;make symbol from num

        mov byte Number[rbx], dl
        dec rbx

		cmp rax, 0	
		jne .next				;while(ax != 0)

.output:

    lea rsi, Number[rbx + 1]        ;message
	mov rcx, MAX_SYMBOL_IN_NUMBER   ;
    sub rcx, rbx                    ;length

	call PutNumberInBuffer

    mov rax, 0x01			        ;write64 (rdi, rsi, rdx) ... r10, r8, r9
	mov rdi, 1				        ;stdout
    lea rsi, Buffer                 ;message to output

    mov rdx, r8                     ;length

	syscall
	ret

;-------------------------------------------
;Put Number in Buffer in right order
;-------------------------------------------
;EXPECTS:	r8 - current index in Buffer
;
;ENTRY:     rcx - length of number
;           rsi - pointer to number
;
;OUTPUT:	r8 - current index in Buffer
;
;DESTROYS:	rdi, rsi, rcx
;-------------------------------------------
PutNumberInBuffer:
    lea rdi, [Buffer + r8]
    cld
    rep movsb

    mov BYTE [rdi], 0xa    ; [rdi] = '\n'
    inc rdi

    mov r8, rdi
    sub r8, Buffer
    ret

;------------------------------------------------
; Get number from stdin
;------------------------------------------------
;EXPECTS:	None
;
;ENTRY:     None
;
;OUTPUT:	None
;
;DESTROYS: rax, rbx, rcx, rdx, rsi, rdi, r8, r10, r11
;------------------------------------------------
InputNumber10:
    xor rax, rax                   ; 0 - syscall number for 'read'
    xor rdi, rdi                   ; 0 - stdin file descriptor
    mov rsi, Number                ;
    mov rdx, MAX_SYMBOL_IN_NUMBER  ;
    syscall                        ; read(0, buffer, BUFSIZE)
                                   ; rax - number of bytes in input
    xor rcx, rcx
    xor r11, r11    

    xor rax, rax
    xor rbx, rbx
    mov r10, 10
    .next:
        inc rcx
        mov bl, Number[rcx - 1]

        cmp bl, '-'             ;
        jne .skip_set_numb_neg_flag;if (bl == '-')
            mov r11, 1
            jmp .skip_add_digit

        .skip_set_numb_neg_flag:
        
        sub bl, '0'     ; make number from symbol
        mul r10         ; rax *= 10
        add rax, rbx    ; rax += rbx
        
        .skip_add_digit
        mov r8, Number[rcx] ;
        cmp r8, 0xa         ;while (Number[rcx] != '\n')  
        jne .next           ;
    .end_calc:

    jne .skip_make_numb_neg
        xor rax, -1
        inc rax

    .skip_make_numb_neg:

    ret
    

section .bss
    BUFFER_LENGTH		 equ 256
    MAX_SYMBOL_IN_NUMBER equ 100

	Buffer:	    db BUFFER_LENGTH*2      dup(?)
    Number:     db MAX_SYMBOL_IN_NUMBER dup(?)  