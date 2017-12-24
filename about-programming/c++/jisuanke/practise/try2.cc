#include <functional>
#include <iostream>

using namespace std;
int main() {
	function<int(int)> inc = [](double x) {return 1 + x;};
	int(*square)(int) = [](int x) { return x * x;};
	cout << inc(2.5) << endl;
	return 0;
}
