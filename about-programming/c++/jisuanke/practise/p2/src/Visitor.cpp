
#include "Visitor.h" // used to get the expression tree structure and output a string
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
    node->First->Accept(this);
    string firstResult{result};
    node->Second->Accept(this);
    result = '(' + firstResult + GetOp(node->Op) + result + ')';
}
void ExprPointer::Visit(InvokeExpr* node)
{
	//TODO
    string tmpResult = '(' + node->name;
    for (auto it : node->arguments) {
       it.Accept(this);
       tmpResult += result;
    }
    result = tmpResult + ')';
}

}
