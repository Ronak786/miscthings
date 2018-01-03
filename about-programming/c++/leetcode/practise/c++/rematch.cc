#include <string>
#include <iostream>
#include <vector>

using namespace std;
// 这个可以处理 aaa ~ ab*a*c*a的情况匹配，但是不能处理 aaca ~ ab*a*c*a 的情况，不行了。。。

enum Type {
    NORMAL,
    DOT,
    DOTSTAR,
    NORMALSTAR
};

struct Matcher {
    using vmiter = vector<Matcher*>::iterator;
    string str;
    Type strType;
    virtual bool match(string::iterator&, string::iterator&, vmiter iter, vmiter vmend) = 0;
    Matcher (string& name, Type type): str(name), strType(type) {}
    ~Matcher () {} 
};

struct DotstarMatcher : Matcher {
    DotstarMatcher (string& name, Type type) : Matcher(name, type) {}
    bool match(string::iterator& bi, string::iterator& ei, vmiter iter, vmiter vmend) {
        // .*xxxxx ,but now just skip all
        bi = ei;
        return true;
    }
};

struct DotMatcher : Matcher {
    DotMatcher (string& name, Type type) : Matcher(name, type) {}
    bool match(string::iterator& bi, string::iterator& ei, vmiter iter, vmiter vmend){ 
        bi++;
        return true;
    }
};

struct NormalstarMatcher : Matcher {
    NormalstarMatcher (string& name, Type type) : Matcher(name, type) {}
    bool match(string::iterator& bi, string::iterator& ei, vmiter iter, vmiter vmend){ 
        // vmiter, vmend here we copy instead of ref
        // while ((*vmiter)->strType == xx*, skip))
        //      if not end and ((*vmiter)->str == orig), then here we not get the last, just return true
       bool flag = false;
       char orig = *bi;
       while (*bi == str[0]) {
           ++bi;
           flag = true;
       }
       while (iter != vmend && ((*iter)->strType == Type::DOTSTAR || (*iter)->strType == Type::NORMALSTAR)) {
           ++iter;
       }
       if (iter != vmend && (*iter)->str[0] == orig && flag) {
           --bi; // reserve one for later match
       }
       return true;
    }
};

struct NormalMatcher : Matcher {
    NormalMatcher (string& name, Type type) : Matcher(name, type) {}
    bool match(string::iterator& bi, string::iterator& ei, vmiter iter, vmiter vmend){ 
        if (str[0] != *bi) return false;
        ++bi;
        return true;
    }
};

struct MatcherFactory {
    Type fillType(string &str) { // fill type according to the string format
        Type strType;
        if (str == ".") {
            strType = Type::DOT;
        } else if (str == ".*") {
            strType = Type::DOTSTAR;
        } else if (str.find('*') == string::npos) {
            strType = Type::NORMAL;
        } else {
            strType = Type::NORMALSTAR;
        }
        return strType;
    }

    Matcher* createMatcher(string& name) {
       switch (fillType(name)) {
           case Type::DOT:
               return new DotMatcher{name, Type::DOT};
           case Type::DOTSTAR:
               return new DotstarMatcher{name, Type::DOTSTAR};
           case Type::NORMALSTAR:
               return new NormalstarMatcher{name, Type::NORMALSTAR};
           case Type::NORMAL:
               return new NormalMatcher{name, Type::NORMAL};
           default:
               throw 0;
       }
    }
};

class Solution {
public:
    //implement re match of '.' and '*' function
    bool isMatch(string s, string p) {
       if (p[0] == '*') return false; // syntax error 
       vector<Matcher*> matchers = splitMatcher(p); // split matchers into array units like a* b* .* . 
       auto iter = begin(s);
       auto tail = end(s);
       bool flag = processMatch(matchers, iter, tail);
       if (flag == false) return false;
//       cout << "now  iter is at " << iter - begin(s) << endl;
       if (iter != end(s)) return false;
       return true;
    }

    vector<Matcher*> splitMatcher(string &p) { // parse patter into a vector of Matcher class
        // we can pass in vector so save one whole vector copy here
        auto cur = begin(p);
        auto tail = end(p);
        MatcherFactory mf;
        vector<Matcher*> vm;
        vm.reserve(p.size());
        while (cur != tail) {
            string tmp = readString(cur, tail);
//        cout << "get string " << tmp << endl;
            vm.push_back(mf.createMatcher(tmp));
        }
        return vm;
    }

    string readString(string::iterator &cur, string::iterator &tail) {
        if (*cur == '*') {
            throw 0; //error !!!
        } else if (*cur == '.') {
            auto curnext = next(cur);
            if (curnext != tail && *curnext == '*') {
                cur += 2;
                return {".*"};
            } else {
                cur++;
                return {"."};
            }
        } else { // cur is normal, return x* or just x
            // here also we can return the non special continue string as a list, not just on char,more effcient
            auto curnext = next(cur);
            if (curnext != tail && *curnext == '*') {
                string tmp{""};
                tmp += *cur;
                tmp += *curnext;
                cur += 2;
                return tmp;
            } else {
                return {*cur++};
            }
        }
    }

    bool processMatch(vector<Matcher*>& matchers, string::iterator& head, string::iterator& tail) {
        //be careful aaab ~ a*ab we must careful the second 'a' !!
        //
        bool flag = false;
        for (auto iter = begin(matchers); iter != end(matchers); ++iter) {
//            cout << "begin matcher " << (*iter)->str << " at string " << string(head, tail) << endl;
            if (!(*iter)->match(head, tail, next(iter), end(matchers)))
                return false;
        }
        return true;
    }
};


int main(int ac, char *av[]) {
    string str{av[1]};
    string pattern{av[2]};
    Solution so;
    cout << so.isMatch(str, pattern) << endl;
    /*
    auto b = begin(str), e = end(str);
    while (b != e) {
        cout << so.readString(b, e) << endl;
    }
    */
    return 0;
}
