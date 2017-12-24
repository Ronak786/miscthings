#include <iostream>

using namespace std;

int a = 0;

int main() {
	auto inc = [&a]() {a++;};
	inc();
	inc();
	cout << a << endl;
	return 0;
}
