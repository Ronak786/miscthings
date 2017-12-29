
#include "Visitor.h" // used to get the expression tree structure and output a string
#include <cstdio>
void ExprPointer::Visit(NumberExpr* node)
{
	//TODO  set result here
    result = to_string(node->number);
}
void ExprPointer::Visit(IdExpr* node)
{
	//TODO
    result = node->name;
}
void ExprPointer::Visit(BinaryExpr* node)
{
	//TODO
    node->first->Accept(this);
    string firstResult{result};
    node->second->Accept(this);
    result = '(' + string() +  GetOp(node->op) + string(" ") + firstResult + string(" ") + result + ')';
}
void ExprPointer::Visit(InvokeExpr* node)
{
	//TODO
    string tmpResult = '(' + node->name;
    for (auto it : node->arguments) {
       it->Accept(this);
       tmpResult += " " + result;
    }
    result = tmpResult + ')';
}

