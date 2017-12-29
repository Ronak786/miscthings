#include "handler.h"
#include <cstdlib>
#include <cctype>
bool IdHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (forInvoke) return false; // for token handler
    if (!std::isalpha(token[0])) return false;  // first letter should alpha
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
            return true;
    }
    return false;
}


BinaryOperator GetToken(char c) {
	switch (c)
	{
	case '+':
		 return BinaryOperator::Plus;
    case '-':
        return BinaryOperator::Minus;
    case '*':
        return BinaryOperator::Multiply;
    case '/':
        return BinaryOperator::Divide;
	}
}

shared_ptr<Expr> BinaryHandler::BinaryFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO must handle recursive call when args > 2
    if (arguments.size() < 2) {throw Exception{};}
    auto ptr = make_shared<BinaryExpr>();
    ptr->op = GetToken(token[0]);
    if (arguments.size() > 2) { 
        ptr->first = arguments[0];
        ptr->second = arguments[1];
        vector<shared_ptr<Expr>> arg2{arguments.begin()+2, arguments.end()};
        arg2.insert(arg2.begin(), ptr);
        ptr = std::dynamic_pointer_cast<BinaryExpr>(CreateExpr(token, arg2));
    } else {
        ptr->first = arguments[0];
        ptr->second = arguments[1];
    }
    return ptr;
}

shared_ptr<Expr> InvokeHandler::InvokeFactory::CreateExpr(string token, vector<shared_ptr<Expr>>arguments)
{
	//TODO
    auto ptr = make_shared<InvokeExpr>();
    ptr->name = token;
    ptr->arguments = arguments;
    return ptr;
}
bool InvokeHandler::Test(string token, bool forInvoke)
{
	//TODO
    if (!forInvoke) return false;
    if (!std::isalpha(token[0])) return false;  // first letter should alpha
    if (std::find_if_not(token.begin(), token.end(), 
                [](auto x){ return std::isalnum(x);}) != token.end()) {
        return false; //should only have alnum
    }
    return true;
}

