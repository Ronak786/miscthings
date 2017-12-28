#include <string>
#include <iostream>
#include <memory>
#include <cassert>
#include <vector>
using namespace std;

const int CAP_NUM = 16;
class Object
{
public:
    string str_object;
	static shared_ptr<Object> getObj();
protected:
	Object(){}

};
struct Position:Object
{
	bool used;
	Position* next;
} buffer[CAP_NUM];
Position* unused ;
Object* createObj()
{
	//TODO
	// unused ptr always chasing the unused link list
	if (unused == nullptr)	
		return nullptr;
	Position *alloc = unused;
    alloc->used = true;
	unused = unused->next;	
	return alloc;
}
bool destroyObj(Object* obj)
{
	//TODO
	Position* pos = static_cast<Position*>(obj); // no check may fault
	if (reinterpret_cast<unsigned long>(pos) < reinterpret_cast<unsigned long>(buffer) 
            || reinterpret_cast<unsigned long>(pos) >= reinterpret_cast<unsigned long>(buffer+CAP_NUM)
            || pos->used == false) // not an effective addr
    {
		return false;
    }
    if (unused == nullptr) {
        pos->next = unused;
        unused = pos;
    } else if (pos - unused < 0){
		pos->next = unused;
		unused = pos;
	} else {
        auto cur = unused;
        while (cur->next != nullptr && cur->next < pos)
            cur = cur->next;
        pos->next = cur->next;
        cur->next = pos;
    }
    pos->used = false;
    return true;
	
}
shared_ptr<Object> Object::getObj()
{
	shared_ptr<Object>obj(createObj(), destroyObj);
	return obj;
}
void bufferInit()
{
	unused = &buffer[0];
	for (int i = 0; i < CAP_NUM-1; i++)
	{
		buffer[i].used = false;
		if (i!=CAP_NUM-1)
		{
			buffer[i].next = &buffer[i + 1];
		}
	}
	buffer[CAP_NUM - 1].next = NULL;
}
void assert_obj(Object* obj)
{
	auto destroy_result = destroyObj(obj);
	assert(destroy_result == false);
}

int main()
{
	bufferInit();
	{
		Object* obj = buffer - sizeof(Position);
		assert_obj(obj);
	}

	{
		int getAddr;
		Object* obj = reinterpret_cast<Object*>(&getAddr);
		assert_obj(obj);
	}

	{
		Object* obj = nullptr;
		assert_obj(obj);
	}

	{
		Object* obj = buffer + CAP_NUM + 1;
		assert_obj(obj);
	}

	{
		Object* obj = createObj();
		destroyObj(obj);
		assert_obj(obj);
	}

	{
		vector<shared_ptr<Object>> vec_obj;
		for (int i = 0; i < CAP_NUM; i++)
		{
			auto p = Object::getObj();
			if (p != nullptr)
			{
				p->str_object = "" + to_string(i);
				cout << p->str_object << endl;
			}
			vec_obj.push_back(p);
		}
		auto p_blank = Object::getObj();
		assert(p_blank == nullptr);

	}
	cout << "all tests done!" << endl;
	return 0;
}
