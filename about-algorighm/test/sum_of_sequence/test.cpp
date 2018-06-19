/*************************************************************************
	> File Name: test.c
	> Author: davee-x
	> Mail: davee.naughty@gmail.com 
	> Created Time: Fri 15 Jun 2018 05:53:18 PM CST
 ************************************************************************/


int sequenceSum(int start, int end, int step) {
	if (end < start) return 0;
	int trueend = end - (end - start) % step;
	int truestart = start;
	int count = (end - start) / step + 1
	return (trueend + truestart)*count /2;
}

int main() {
	
}
