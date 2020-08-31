		extern AllocConsole
		extern GetStdHandle
		extern WriteConsoleA

		default rel
		section .text
		global main
main:
		push rbp
		nop
		lea rax, [rsp+4]
		mov dword[rax], 3
		call AllocConsole
		mov rcx, -11		; STD_OUTPUT_HANDLE
		call GetStdHandle
		mov rcx, rax
		lea rdx, [text]
		mov r8, 12
		lea r9, [n]
		push 0
		call WriteConsoleA
		pop rbp
		ret

n		dd 0				; # of chars written
text	db 'Hello World!', 0