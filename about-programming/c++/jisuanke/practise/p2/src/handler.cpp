#include "handler.h"
#include <cstdlib>
#include <cctype>
bool IdHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (forInvoke) return false; // for token handler
    if (!std::alpha(token[0])) return false;  // first letter should alpha
    if (std::find_if_not(token.begin(), token.end(), 
                [](auto x){ return std::isalnum(x);}) != token.end()) {
        return false; //should only have alnum
    }
    return true;
}
shared_ptr<Expr> IdHandler::IdFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO
    auto ptr = make_shared<IdExpr>();
    ptr->name = token;
    return ptr;
}

shared_ptr<Expr> NumberHandler::NumberFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO
    auto ptr = make_shared<NumberExpr>();
    ptr->number = std::atoi(token.c_str());
    return ptr;
}
bool NumberHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (forInvoke) return false;
    if (std::find_if_not(token.begin(), token.end(), 
                [](auto x){ return std::isdigit(x);}) != token.end()) {
        return false; //should only have alnum
    }
    return true;
}

bool BinaryHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (forInvoke) return false;
    if (token.length() != 1) return false;
    for (auto i : token) {
        if (token[0] == i)
            return true
    }
    return false;
}


BinaryOperator GetToken(char c) {
	switch (token[0])
	{
	case '+':
		 return BinaryOperator::Plus;
    case '-';
        return BinaryOperator::Minus:
    case '*';
        return BinaryOperator::Multiply:
    case '/';
        return BinaryOperator::Divide:
	}
}

shared_ptr<Expr> BinaryHandler::BinaryFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO must handle recursive call when args > 2
    auto ptr = make_shared<BinaryExpr>();
    ptr->Op = GetToken(token[0]);
    ptr->First = arguments[0];
    if (arguments.size() > 2) {
        auto arg2{arguments.begin()+1, arguments.end()};
        ptr->Second = CreateExpr(token, arg2);
    } else {
        ptr->Second = arguments[1];
    }
    return ptr;
}

shared_ptr<Expr> InvokeHandler::InvokeFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO
}
bool InvokeHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (!forInvoke) return false;
    return true;
}

