extern int MessageBoxA(
	int   hWnd,
	char* lpText,
	char* lpCaption,
	int   uType
);

void main()
{
	char text[] = {'t', 'e', 'x', 't', '\0'};

	MessageBoxA(0, text, text, 0);
}