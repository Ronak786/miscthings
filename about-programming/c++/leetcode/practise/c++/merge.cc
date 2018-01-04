#include <iostream>
struct ListNode {
    int val;
    ListNode *next;
    ListNode(int x) : val(x), next(NULL) {}
};

class Solution {
public:
    ListNode* mergeTwoLists(ListNode* l1, ListNode* l2) {
        ListNode *head = new ListNode{0};
        ListNode *cur = head;
 		while (l1 != nullptr && l2 != nullptr){ 
		      ListNode * tmp1 = l1->next; 
              ListNode * tmp2 = l2->next; 
              if (l1->val < l2->val) {
                  cur->next = l1;
                  l1->next = nullptr;
                  cur = l1;
                  l1 = tmp1;
              } else {
                  cur->next = l2;
                  l2->next = nullptr;
                  cur = l2;
                  l2 = tmp2;
              }
        }
        if (l1 != nullptr) {
            cur->next = l1;
        } else {
            cur->next = l2;
        }
        cur = head;
        head = head->next;
        delete cur;
        return head;
    }
};

/*
        ListNode dummy(INT_MIN);
        ListNode *tail = &dummy;
        
        while (l1 && l2) {
            if (l1->val < l2->val) {
                tail->next = l1;
                l1 = l1->next;
            } else {
                tail->next = l2;
                l2 = l2->next;
            }
            tail = tail->next;
        }

        tail->next = l1 ? l1 : l2;
        return dummy.next;
*/ 
// 多分的那个会作为auto变量自动回收
int main() {return 0;}
