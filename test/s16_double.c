#include <unistd.h>

int main()
{
	short s;
	double d;
	while( read(STDIN_FILENO, &s, sizeof(short)) == sizeof(short) )
	{
		d = (double)s / 32768.; /* funny: 32767 makes mpg123's s16 compare better with reference */
		write(STDOUT_FILENO, &d, sizeof(double));
	}
	return 0;
}
