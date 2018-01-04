#include <iostream>
#include <vector>

using namespace std;
struct ListNode {
    int val;
    ListNode *next;
    ListNode(int x) : val(x), next(NULL) {}
};

class Solution {
public:
    ListNode* removeNthFromEnd(ListNode* head, int n) {
    	vector<ListNode**> vl;
		ListNode **cur = &head->next;
		vl.push_back(&head);
		while (*cur) {
            vl.push_back(cur);
            cur = &(*cur)->next;
        }
        auto ptr = vl[vl.size()-n];
        auto save = *ptr;
        *ptr = (*ptr)->next;
        delete save;
        return head;
    }

    void printList(ListNode* head) {
        while (head) {
            cout << head->val << " ";
            head = head->next;
        }
        cout << endl;
    }

    ListNode* constructList(vector<int> vi) {
        ListNode *head = nullptr, *cur;
        for (int i = 0; i < vi.size(); ++i) {
            if (!head)  {
                head = new ListNode{vi[i]};
                cur = head;
            } else {
                cur->next = new ListNode{vi[i]};
                cur = cur->next;
            }
        }
        return head;
    }
};
/*
//还是走的快的点(fastNode)与走得慢的点(slowNode)路程差的问题
	public static ListNode removeNthFromEnd(ListNode head, int n) {
        ListNode headNode = new ListNode(9527);
        headNode.next = head;
        ListNode fastNode = headNode;
        ListNode slowNode = headNode;
        while(fastNode.next != null){
        	if(n <= 0)
        		slowNode = slowNode.next;
        	fastNode = fastNode.next;
        	n--;
        }
        if(slowNode.next != null)
        	slowNode.next = slowNode.next.next;
        return headNode.next;
    }
*/  
// 通过一个节点遍历到结尾造成的 target - size() = -应该走的路程,前面加一个节点，就找到了他之前的那个位置
int main() {
    Solution so;
    vector<int> vi{1,2,3,4,5};
    ListNode *head = so.constructList(vi);
    so.printList(head);
    head = so.removeNthFromEnd(head, 5);
    so.printList(head);
    return 0;
}
