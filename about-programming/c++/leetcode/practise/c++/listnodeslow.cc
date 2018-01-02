#include <vector>
#include <iostream>
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
            int tmpres =  0;
            ListNode *head{nullptr}, *tail{nullptr};
            while (l1 != NULL && l2 != NULL) {
                tmpres = l1->val + l2->val + inc;
                inc = tmpres / 10;
                if (!head) {
                    head = tail = new ListNode{tmpres%10};
                } else {
                    tail->next = new ListNode(tmpres%10);
                    tail = tail->next;
                }
                l1 = l1->next;
                l2 = l2->next;
            }
            while (l1 != NULL) {
                tmpres = l1->val + inc;
                inc = tmpres / 10;
                if (!head) {
                    head = tail = new ListNode{tmpres%10};
                } else {
                    tail->next = new ListNode(tmpres%10);
                    tail = tail->next;
                }
                l1 = l1->next;
            }
            while (l2 != NULL) {
                tmpres = l2->val + inc;
                inc = tmpres / 10;
                if (!head) {
                    head = tail = new ListNode{tmpres%10};
                } else {
                    tail->next = new ListNode(tmpres%10);
                    tail = tail->next;
                }
                l2 = l2->next;
            }
            if (inc) {
                tail->next = new ListNode{1};
            }
            return head;
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
