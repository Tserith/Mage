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

	int factorial = 0;

	for (int i = 0; i < 10; i++)
	{
		factorial *= 10 - i;
	}
}