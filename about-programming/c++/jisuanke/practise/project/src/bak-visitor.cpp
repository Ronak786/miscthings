#include "visitor.h"
#include <sstream>
/*
 * 还有一种方法，用一个栈，从中间开始经历过就ｐｕｓｈ，然后对左边设置ｌｅｆｔ，然后做左边
 * 然后做右边，这个没有ｌｅｆｔ，然后把左右和自己组合起来，
 * 然后判断自己是否左边，以及自己和栈顶的比较，来看自己要不要加括号
 * 然后ｐｏｐ栈顶, 这个比我这个好，因为不需要任何cast判断
 */
//TODO
void ExpressionPointer::Visit(NumberExpression* node)
{
    result = std::to_string(node->Value);
}


void ExpressionPointer::Visit(BinaryExpression* node)
{

    // do with left
    BinaryExpression* left = dynamic_cast<BinaryExpression*>(node->First.get());
    if (left != nullptr) {
       Visit(left);
       if (Regular(static_cast<int>(node->Op)) 
               && !Regular(static_cast<int>(left->Op))) { // left is +- , middle is */
           result = "(" + result + ")" + string("") + GetOp(node->Op);
       } else { // other circuments
           result = result + GetOp(node->Op);
       }
    } else { // left is just a number
           Visit(dynamic_cast<NumberExpression*>(node->First.get()));
           result = result + GetOp(node->Op);
    }

    string halfResult{result}; // the second half process will rewrite result variable, so save it first

    // do with right
    BinaryExpression* right = dynamic_cast<BinaryExpression*>(node->Second.get());
    if (right != nullptr) { // right is an expression
        Visit(right);
        BinaryOperator rightOp = right->Op;
        BinaryOperator middleOp = node->Op;

        if ((middleOp == BinaryOperator::Divide) || 
                ((middleOp == BinaryOperator::Multiply || middleOp == BinaryOperator::Minus) 
                  && (rightOp == BinaryOperator::Plus || rightOp == BinaryOperator::Minus)))
        {
            result = halfResult + string("") + "(" + result + ")";
        } else {
            result = halfResult + result;
        }
    } else {
        Visit(dynamic_cast<NumberExpression*>(node->Second.get()));
        result = halfResult + result;
    }
}
