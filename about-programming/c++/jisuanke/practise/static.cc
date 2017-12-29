#include <iostream>
#include <limits>
#include <cstdlib>
using namespace std;

class Screen {
private:
  	unsigned int _width;
  	unsigned int _height;
    static Screen* instance;

  	Screen(unsigned int width, unsigned int height) {
    		_width = width;
    		_height = height;
  	};
  	Screen() {

  	}
  	~Screen() {
  		delete instance;
  	}
public:
  	//TODO
    Screen(const Screen&)=delete;
    Screen & operator= (const Screen&)=delete;
    static Screen* getInstance(int width, int height);
    static Screen* getInstance();
    int getWidth() {
        return _width;
    }

    int getHeight() {
        return _height;
    }
};

Screen* Screen::instance = nullptr;

Screen* Screen::getInstance(int width, int height) {
    if (instance == nullptr) {
        if (width <= 0 || width > 1920)
            width = 1920;
        if (height <= 0 || height > 1080)
            height = 1080;
        instance = new Screen(width, height);
    }
    return instance;
}

Screen* Screen::getInstance() {
    if (instance == nullptr) 
        return getInstance(1920, 1080);
    return instance;
}


int main() {
  	int width, height;
  	Screen* screen = 0;
  	cin >> width >> height;
  	screen = Screen::getInstance(width, height);
  	screen = Screen::getInstance();
  	cout << screen->getWidth() << " " <<
  		screen->getHeight() << endl;

  	return 0;
}

