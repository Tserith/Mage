		global start
		
		extern MessageBoxA
		extern ExitProcess

		section .text
start:	
		; probably should set rsp and rbp to top of stack
		sub rsp, 28h ; stack space required for function calls
		mov r9, 0
		lea r8, [rel title]
		lea rdx, [rel text]
		mov rcx, 0
		call MessageBoxA
		xor rcx, rcx
		call ExitProcess

		title	db 'Message Box', 0
		text	db 'Hello World!', 0
end: