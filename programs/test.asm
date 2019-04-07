		global start ; entry point

		extern MessageBoxA
		extern ExitProcess

		section .text
start:	
		sub rsp, 28h ; stack space required for function calls
		mov r9, 0
		mov r8, title
		mov rdx, text
		mov rcx, 0
		call MessageBoxA
		xor rcx, rcx
		call ExitProcess

		section	.data
title	db 'Message Box', 0
text	db 'Hello World!', 0