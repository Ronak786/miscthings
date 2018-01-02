#include <vector>
#include <iostream>
/*
 * use existing listnode instead of alloc new ones
 * and pre alloc a struct to get rid of the exception "if-else" code from function
 * (the assignment of tail->next=l1 is not nesessary because we are using that chain,
 *  but when change from one list to the other, it is necessary, and only l1->l2 is possible in our code
 */
using namespace std;
struct ListNode {
    int val;
    ListNode *next;
    ListNode(int x): val(x), next(NULL) {}
};

class Solution {
    public:
        ListNode* addTwoNumbers(ListNode* l1, ListNode* l2) {
            int inc = 0;
            ListNode *head, *tail;
            head = tail = new ListNode{0};
            head->next = l1? l1:l2;

            while (l1 != NULL && l2 != NULL) {
                l1->val = l1->val + l2->val + inc;
                inc = l1->val / 10;
                l1->val %= 10;
                tail = l1;
                l1 = l1->next;
                l2 = l2->next;
            }
            while (l1 != NULL) {
                l1->val = l1->val + inc;
                inc = l1->val / 10;
                l1->val %= 10;
                tail = l1;
                l1 = l1->next;
            }
            while (l2 != NULL) {
                l2->val = l2->val + inc;
                inc = l2->val / 10;
                l2->val %= 10;
                tail->next = l2;
                tail = l2;
                l2 = l2->next;
            }
            if (inc) {
                tail->next = new ListNode{1};
            }
            tail = head->next;
            free(head);
            return tail;
        }

        ListNode* constructNode(vector<int>& vi) {
            ListNode *head(nullptr), *tail(nullptr);

            for (auto i : vi) {
                if (!head) {
                    head = tail = new ListNode{i};
                } else {
                    tail->next = new ListNode{i};
                    tail = tail->next;
                }
            }
            return head;
        }
};


int main() {
    Solution so;
    vector<int> one{1,8}, two{0};
    ListNode * l1 = so.constructNode(one);
    ListNode * l2 = so.constructNode(two);
    ListNode * res = so.addTwoNumbers(l1, l2);

    while (res) {
        cout << res->val << endl;
        res = res->next;
    }
    return 0;
}
