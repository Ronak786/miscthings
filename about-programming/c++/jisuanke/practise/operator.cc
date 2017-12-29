#include <iostream>
using namespace std;
class Complex
{
private:
  	double real;
  	double image;
public:
  	Complex(const Complex& complex) :real{ complex.real }, image{ complex.image } {

  	}
  	Complex(double Real=0, double Image=0) :real{ Real }, image{ Image } {

  	}
  	//TODO
    friend ostream& operator<<(ostream &o, const Complex& c);
    friend istream& operator>>(istream &i, Complex& c);
    Complex operator+(const Complex& c);
    Complex operator-(const Complex& c);
    Complex operator*(const Complex& c);
    Complex operator/(const Complex& c);
};

ostream& operator<<(ostream &o, const Complex& c) {
    o << "(" << c.real;
    if (c.image >= 0)
        o << "+";
    o << c.image << "i)";
    return o;
}

istream& operator>>(istream &i, Complex& c) {
    cout << "in read in" << endl;
    i >> c.real >> c.image;
    cout << "out read in" << endl;
    return i;
}

Complex Complex::operator+(const Complex& c) {
    return {real + c.real, image + c.image};
}

Complex Complex::operator-(const Complex& c) {
    return {real - c.real, image - c.image};
}

Complex Complex::operator*(const Complex& c) {
    double tmpreal = real * c.real - image * c.image;
    double tmpimage = image * c.real + real * c.image;
    return {tmpreal, tmpimage};
}

Complex Complex::operator/(const Complex& c) {
    double tmpreal = (real*c.real + image*c.image) / (c.real*c.real + c.image*c.image);
    double tmpimage = (image*c.real - real*c.image) / (c.real*c.real + c.image*c.image);
    real = tmpreal;
    image = tmpimage;
    return {tmpreal, tmpimage};
}

int main() {
  	Complex z1, z2;
  	cin >> z1;
  	cin >> z2;
  	cout << z1 << " " << z2 << endl;
  	cout << z1 + z2 << endl;
  	cout << z1 - z2 << endl;
  	cout << z1*z2 << endl;
  	cout << z1 / z2 << endl;
  	return 0;
}
