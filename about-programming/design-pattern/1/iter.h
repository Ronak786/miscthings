#ifndef ITER_H_
#define ITER_H_

#include <string>
#include <iostream>
#include <vector>

class Iter {
    public:
        typedef std::vector<std::string>::iterator iterator;
        void AddItem(std::string item) {
            vs_.push_back(item);
        }
        iterator begin() { return vs_.begin(); }
        iterator end() { return vs_.end(); }
    private:
        std::vector<std::string> vs_;
};

#endif
