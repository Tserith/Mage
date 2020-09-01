		extern MessageBoxA

		default rel
		section .text
		global main
main:
		push rbp

		mov rcx, 0
		lea rdx, [text]
		lea r8, [title]
		mov r9, 0
		call MessageBoxA

		pop rbp
		ret

title	db 'Message Box', 0
text	db 'Hello World!', 0