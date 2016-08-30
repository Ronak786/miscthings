#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static int binary_search(int index, int low, int high);
static int bin_search_nore(int index, int low, int high);

int main(int ac, char *av[])
{
	int low, high, index, ret;
	
	if (ac != 4) {
		printf("index, low, high\n");
		exit(1);
	}

	index = strtol(av[1], NULL, 10);
	low = strtol(av[2], NULL, 10);
	high = strtol(av[3], NULL, 10);

	ret = bin_search_nore(index, low, high);
	printf("have %s found index %d\n", ret != 0 ? "" : "not", index);
	return 0;
}


static int binary_search(int index, int low, int high)
{
	int m, l, h;
	static int count = 0;

	if (low > high) {
		printf("not found , after %d loop\n", ++count);
		return 0;
	}

	l = low;
	h = high;
	m = l + (h - l)/2;
	if (m < index) {
		l = m + 1;
		count++;
		printf("next from %d to %d\n", l, h);
		return binary_search(index, l, h);
	}
	if (m > index) {
		h = m - 1;
		count++;
		printf("next from %d to %d\n", l, h);
		return binary_search(index, l, h);
	}
	printf("in %d to %d , found %d as middle, count is %d\n", l, h, index, ++count);
	return 1;
}

static int bin_search_nore(int index, int low, int high)
{
	int m, l, h, count=0;

	l = low;
	h = high;

	while (l <= h) 
	{
		m = l + (h - l)/2;
		if (m < index) {
			l = m + 1;
			count++;
			printf("next from %d to %d\n", l, h);
			return bin_search_nore(index, l, h);
		}
		if (m > index) {
			h = m - 1;
			count++;
			printf("next from %d to %d\n", l, h);
			return bin_search_nore(index, l, h);
		}
		printf("in %d to %d, found %d as middle, count is %d\n", l, h, m, ++count);
		return 1;
	}
	printf("in %d to %d ,not  found  middle, count is %d\n", l, h, ++count);
	return 0;
}
