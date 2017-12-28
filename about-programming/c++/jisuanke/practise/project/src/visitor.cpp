#include "visitor.h"
#include <iostream>
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
   stack_bo.push(node->Op); // push current operator

   //check the left child
   node->First->IsLeft = true;
   auto left = dynamic_cast<BinaryExpression*>(node->First.get());
   if (left == nullptr) 
       Visit(dynamic_cast<NumberExpression*>(node->First.get())); 
   else
       Visit(left);
   string leftResult = result + GetOp(node->Op);

   // check the right child
   auto right = dynamic_cast<BinaryExpression*>(node->Second.get());
   if (right == nullptr) 
       Visit(dynamic_cast<NumberExpression*>(node->Second.get())); 
   else
       Visit(right);
   stack_bo.pop(); // pop current operator
   result = leftResult + result;

   // check current according the isleft flag and top of stack(parent)
   if (stack_bo.size() != 0) { // not the root, so we should check
       BinaryOperator parentOp = stack_bo.top();
       if (node->IsLeft) {  // current is left child
           if ((node->Op == BinaryOperator::Plus || node->Op == BinaryOperator::Minus)
                   && (parentOp == BinaryOperator::Multiply || parentOp == BinaryOperator::Divide)) {
               result = "(" + result + ")";
           }
       } else { // is right child
           if ((parentOp == BinaryOperator::Divide)
                   || ((parentOp == BinaryOperator::Minus || parentOp == BinaryOperator::Multiply)
                       && (node->Op == BinaryOperator::Plus || node->Op == BinaryOperator::Minus))) {
                result = "(" + result + ")";
           }
       }
   }
}
