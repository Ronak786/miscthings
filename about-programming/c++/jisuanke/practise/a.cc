#include <iostream>
#include <string>
#include <memory>

using namespace std;

struct Node
{
    int data;

private:
    Node() = default;
    ~Node() = default;
    template<class T, class... TArgs>
    friend shared_ptr<T> std::make_shared(TArgs&&... args);
};
namespace std {
    template<>
    shared_ptr<Node> make_shared<Node>() {
        return shared_ptr<Node>(new Node(), [](Node* node) {delete node;});
    }
}
int main()
{
    auto node = make_shared<Node>();
    return 0;
}
