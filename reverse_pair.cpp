#include <vector>
#include <algorithm>
#include <set>
#include <assert.h>
#include <memory>
using namespace std;

class RBTreeNode {
    private:
        std::shared_ptr<RBTreeNode> left, right;
        std::weak_ptr<RBTreeNode> parent;
        bool red;
        size_t num_reach;
        int value;

        void update_num_reach()
        {
            size_t n = 1;
            if (this->left) n += this->left->num_reach;
            if (this->right) n += this->right->num_reach;

            if (this->num_reach != n) {
                this->num_reach = n;
                auto parent = this->parent.lock();
                if (parent) parent->update_num_reach();
            }
        }

        static std::shared_ptr<RBTreeNode> rootnode(std::shared_ptr<RBTreeNode> node) {
            while (node->parent.lock()) {
                node = node->parent.lock();
            }

            return node;
        }

        static void insert_into_uncheck(std::shared_ptr<RBTreeNode> root, std::shared_ptr<RBTreeNode> node)
        {
            auto cn = root;
            for(;;) {
                if (node->value < cn->value) {
                    if (cn->left == nullptr) {
                        cn->left = node;
                        node->parent = cn;
                        break;
                    } else {
                        cn = cn->left;
                    }
                } else {
                    if (cn->right == nullptr) {
                        cn->right = node;
                        node->parent = cn;
                        break;
                    } else {
                        cn = cn->right;
                    }
                }
            }
        }

        static std::shared_ptr<RBTreeNode> fix_adjacent_black_nodes(std::shared_ptr<RBTreeNode> node) {
            auto parent = node->parent.lock();
            assert(parent != nullptr);
            int rotate_case = parent->left == node ? 0 : 1;
            auto grandparent = parent->parent.lock();
            assert(grandparent != nullptr);
            rotate_case += grandparent->left == parent ? 0 : 2;
            auto ggp = grandparent->parent.lock();
            bool gp_be_left = ggp && ggp->left == grandparent;

            std::vector<std::shared_ptr<RBTreeNode>> need_update_num;
            auto n3 = node;
            auto n6 = n3->left;
            auto n7 = n3->right;
            auto new_top = n3;
            switch (rotate_case) {
            // left left
            case 0: {
                auto n1 = grandparent;
                auto n2 = parent;
                auto n4 = n1->right;
                auto n5 = n2->right;
                n2->left = n3;
                n3->parent = n2;
                n2->right = n1;
                n1->parent = n2;
                n1->left = n5;
                if (n5) n5->parent = n1;
                n1->right = n4;
                if (n4) n4->parent = n1;
                n3->red = true;
                need_update_num.push_back(n1);
                need_update_num.push_back(n3);

                new_top = n2;
            } break;
            // left right
            case 1: {
                auto n1= grandparent;
                auto n2 = parent;
                auto n4 = n1->right;
                auto n5 = n2->left;
                n3->left = n2;
                n2->parent = n3;
                n3->right = n1;
                n1->parent = n3;
                n2->right = n6;
                if (n6) n6->parent = n2;
                n1->left = n7;
                if (n7) n7->parent = n1;
                n2->red = true;
                need_update_num.push_back(n1);
                need_update_num.push_back(n2);
            } break;
            // right left
            case 2: {
                auto n1 = grandparent;
                auto n2 = parent;
                auto n4 = n1->left;
                auto n5 = n2->right;
                n3->left = n1;
                n1->parent = n3;
                n3->right = n2;
                n2->parent = n3;
                n1->right = n6;
                if (n6) n6->parent = n1;
                n2->left = n7;
                if (n7) n7->parent = n2;
                n2->red = true;
                need_update_num.push_back(n1);
                need_update_num.push_back(n2);
            } break;
            // right right
            case 3: {
                auto n1 = grandparent;
                auto n2 = parent;
                auto n4 = n1->left;
                auto n5 = n2->left;
                n2->left = n1;
                n1->parent = n2;
                n2->right = n3;
                n3->parent = n2;
                n1->left = n4;
                if (n4) n4->parent = n1;
                n1->right = n5;
                if (n5) n5->parent = n1;
                n3->red = true;
                need_update_num.push_back(n1);
                need_update_num.push_back(n3);

                new_top = n2;
            } break;
            default:
                assert(false && "impossible");
            }

            new_top->parent = ggp;
            if (ggp) {
                if (gp_be_left)
                    ggp->left = new_top;
                else
                    ggp->right = new_top;

                if (!ggp->red) {
                    assert(!new_top->red);
                    for (auto n: need_update_num) n->update_num_reach();
                    return fix_adjacent_black_nodes(new_top);
                }
            } else {
                new_top->red = true;
            }
            for (auto n: need_update_num) n->update_num_reach();

            return new_top;
        }


    public:
        RBTreeNode(int val): red(true), num_reach(1), value(val) {}

        int val() const { return this->value; }

        static std::shared_ptr<RBTreeNode> insert(std::shared_ptr<RBTreeNode> root, int val) {
            assert(root->parent.lock() == nullptr);

            auto new_node = make_shared<RBTreeNode>(val);
            new_node->red = false;
            RBTreeNode::insert_into_uncheck(root, new_node);

            auto p1 = new_node->parent.lock();
            assert(p1 != nullptr);
            if (!p1->red) {
                auto top = fix_adjacent_black_nodes(new_node);
                if (top->parent.lock() == nullptr)
                    root = top;
            } else {
                p1->update_num_reach();
            }

            return root;
        }

        size_t pos() const {
            size_t ans = 0;
            for (auto repr_node = this;repr_node!=nullptr;) {
                if (repr_node->left != nullptr) {
                    ans += repr_node->left->size();
                }

                auto parent = repr_node->parent.lock();
                while (parent && parent->left.get() == repr_node) {
                    repr_node = parent.get();
                    parent = parent->parent.lock();
                }

                repr_node = parent.get();
                if (repr_node != nullptr) ans++;
            }

            return ans;
        }

        RBTreeNode* lower_bound(int val) const {
            assert(this->parent.lock() == nullptr);
            RBTreeNode* ans = val <= this->value? const_cast<RBTreeNode*>(this) : nullptr;

            for (RBTreeNode* node=const_cast<RBTreeNode*>(this);node!=nullptr;) {
                if (val <= node->value) {
                    if (ans == nullptr || node->value <= ans->value) {
                        ans = node;
                    }

                    node = node->left.get();
                } else {
                    node = node->right.get();
                }
            }

            return ans;
        }

        RBTreeNode* upper_bound(int val) const {
            assert(this->parent.lock() == nullptr);
            RBTreeNode* ans = val < this->value ? const_cast<RBTreeNode*>(this) : nullptr;

            for (RBTreeNode* node=const_cast<RBTreeNode*>(this);node!=nullptr;) {
                if (val < node->value) {
                    if (ans == nullptr || node->value <= ans->value) {
                        ans = node;
                    }

                    node = node->left.get();
                } else {
                    node = node->right.get();
                }
            }

            return ans;
        }

        static std::shared_ptr<RBTreeNode> leftmost(std::shared_ptr<RBTreeNode> node) {
            while (node->left)
                node = node->left;

            return node;
        }

        static std::shared_ptr<RBTreeNode> rightmost(std::shared_ptr<RBTreeNode> node) {
            while (node->right)
                node = node->right;

            return node;
        }

        std::shared_ptr<RBTreeNode> next() {
            if (this->right)
                return leftmost(this->right);

            auto nc = this;
            for(;;) {
                auto parent = nc->parent.lock();
                if (!parent) return nullptr;

                if (parent->left.get() == nc) {
                    return parent;
                } else {
                    nc = parent.get();
                }
            }

            return nullptr;
        }

        std::shared_ptr<RBTreeNode> pre() {
            if (this->left)
                return rightmost(this->left);

            auto nc = this;
            for(;;) {
                auto parent = nc->parent.lock();
                if (!parent) return nullptr;

                if (parent->right.get() == nc) {
                    return parent;
                } else {
                    nc = parent.get();
                }
            }

            return nullptr;
        }

        size_t size() {
            return this->num_reach;
        }
};

class RBTreeSet {
    private:
        std::shared_ptr<RBTreeNode> root;

    public:
        // TODO copy constructor

        RBTreeSet() {}
        void insert(int val) {
            if (this->root == nullptr) {
                this->root = std::make_shared<RBTreeNode>(val);
            } else {
                this->root = RBTreeNode::insert(this->root, val);
            }
        }

        RBTreeNode* lower_bound(int val) const {
            if (this->root == nullptr) return nullptr;
            return this->root->lower_bound(val);
        }

        RBTreeNode* upper_bound(int val) const {
            if (this->root == nullptr) return nullptr;
            return this->root->upper_bound(val);
        }

        std::shared_ptr<RBTreeNode> begin() {
            return this->root ? RBTreeNode::leftmost(this->root) : nullptr;
        }

        std::shared_ptr<RBTreeNode> end() {
            return nullptr;
        }

        size_t size() {
            return this->root ? this->root->size() : 0;
        }
};

class Solution {
public:
    int reversePairs(vector<int>& nums) {
        int res =  0;
        vector<int> sorted_nums = nums;
        std::sort(sorted_nums.begin(), sorted_nums.end());
        RBTreeSet saw_nums;
        const auto divideByTwoL = [](int v) { return v % 2 != 0 ? (v < 0 ? v / 2 - 1 : v / 2) : v / 2; };

        for (const int v: nums) {
            saw_nums.insert(v);
            const auto vo2 = divideByTwoL(v);

            auto where = v % 2 == 0 
                ? std::lower_bound(sorted_nums.begin(), sorted_nums.end(), vo2) 
                : std::upper_bound(sorted_nums.begin(), sorted_nums.end(), vo2);
            auto dis = std::distance(sorted_nums.begin(), where);

            auto w2 = v % 2 == 0
                ? saw_nums.lower_bound(vo2)
                : saw_nums.upper_bound(vo2);
            auto d2 = w2 ? w2->pos() : saw_nums.size();
            assert(dis >= d2);
            dis -= d2;

            res += dis;
        }

        return res;
    }
};

#include <random>
#include <iostream>
default_random_engine def_eng;

void testcase() {
    std::uniform_int_distribution<int> dist;
    RBTreeSet nums;
    vector<int> l1;
    for (size_t i=0;i<50000;i++) {
        auto val = dist(def_eng);
        l1.push_back(val);
        auto s = nums.size();
        nums.insert(val);
        assert(s + 1 == nums.size());
    }
    std::sort(l1.begin(), l1.end());

    auto n1 = nums.begin();
    for (size_t i=0;i<l1.size();i++) {
        const auto v = l1[i];
        if (v != n1->val()) {
            cout << v << " " << n1->val() << endl;
        }
        auto vl = nums.lower_bound(v);
        auto vu = nums.upper_bound(v);
        assert(vl == n1.get());
        assert(n1->pos() == i);
        assert(v == n1->val());
        n1 = n1->next();
        assert(vu == n1.get());
    }

    assert(n1 == nullptr);
}

int main() {
    Solution s;
    vector<int> list;

    auto ans1 = s.reversePairs(list);
    cout << ans1 << endl;;
    return 0;
}
