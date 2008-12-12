#include <unistd.h>

int main()
{
	float f;
	double d;
	while( read(STDIN_FILENO, &f, sizeof(float)) == sizeof(float) )
	{
		d = f;
		write(STDOUT_FILENO, &d, sizeof(double));
	}
	return 0;
}
