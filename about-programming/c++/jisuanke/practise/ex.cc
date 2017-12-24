#include <string>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

struct Exception
{
};

struct Bird
{
    int id = 0;

    Bird()
    {
    }

    Bird(int theId, bool requireThrow = false)
        :id{ theId }
    {
        if (requireThrow)
        {
            throw Exception{};
        }
    }

    Bird(Bird&& bird)
        :id{ bird.id }
    {
    }

    Bird(const Bird& bird)
        :id{ bird.id }
    {
    }

    ~Bird()
    {
    }

    Bird& operator=(Bird&& bird)
    {
        id = bird.id;
        return *this;
    }

    Bird& operator=(const Bird& bird)
    {
        id = bird.id;
        return *this;
    }
};

int main()
{
    vector<Bird> birds;
    birds.reserve(5);
    birds.emplace_back(1);
    birds.emplace_back(2);
    birds.emplace_back(3);
    birds.emplace_back(4);

    try
    {
        birds.emplace(begin(birds) + 2, 5, true);
    }
    catch (Exception)
    {
    }

    for (auto& bird : birds)
    {
        cout << bird.id << " ";
    }
    cout << endl;
    return 0;
}
