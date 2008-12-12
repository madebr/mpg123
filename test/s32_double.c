#include <unistd.h>
#include <stdint.h>

int main()
{
	int32_t s;
	double d;
	if(sizeof(int32_t) != 4) return 1;

	while( read(STDIN_FILENO, &s, sizeof(int32_t)) == sizeof(int32_t) )
	{
		d = (double)s / 2147483648.;
		write(STDOUT_FILENO, &d, sizeof(double));
	}
	return 0;
}
