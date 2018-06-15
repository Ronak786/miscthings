#include <stdinc.h>
#include <math.h>

int main(void)
{
	double t = 9.0;
	while (fabs(t - 9.0/t) > 0.001 ) {
		t = (9.0/t + t) / 2.0;
	}
	printf(" t %0.5f, dividor %0.5f\n", t, 9.0/t);
	printf(" %0.5f\n", t);
}
	

	
