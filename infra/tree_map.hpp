#pragma once

#include "pair.hpp"
#include "printer.hpp"

template <typename K, typename V>
class TreeNode {
public:
    K key;
    V value;
    TreeNode *fa;
    TreeNode *left;
    TreeNode *right;
    int height;

    TreeNode(const K &key, const V &value) : key(key), value(value), fa(NULL), left(NULL), right(NULL), height(1) {}

    void update_height() {
        if (!left && !right) height = 1;
        else if (!left) height = right->height + 1;
        else if (!right) height = left->height + 1;
        else height = (left->height > right->height ? left->height : right->height) + 1;
    }

    int balance_factor() {
        if (!left && !right) return 0;
        else if (!left) return -right->height;
        else if (!right) return left->height;
        else return left->height - right->height;
    }

    TreeNode operator=(const TreeNode &other) {
        key = other.key;
        value = other.value;
        if (other.left) {
            left = new TreeNode(other.left.key, other.left.value);
            *left = other->left;
        }
        if (other.right) {
            right = new TreeNode(other.right.key, other.right.value);
            *right = other->right;
        }
        return *this;
    }

    TreeNode(const TreeNode &other) {
        key = other.key;
        value = other.value;
        if (other.left) {
            left = new TreeNode(other.left.key, other.left.value);
            *left = other->left;
        }
        if (other.right) {
            right = new TreeNode(other.right.key, other.right.value);
            *right = other->right;
        }
    }

    ~TreeNode() {
        delete left;
        delete right;
    }
};

template <typename K, typename V>
class TreeMap {
private:
    TreeNode<K, V> *root = NULL;

    void left_rotate(TreeNode<K, V> *l)
    {
        TreeNode<K, V> *r = l->right, *p = l->fa;
        if (!r) return;
        l->right = r->left;
        r->left = l;

        r->fa = p;
        if (p) {
            if (p->left == l) p->left = r;
            else p->right = r;
        } else root = r;

        l->fa = r;
        if (l->right) l->right->fa = l;

        l->update_height();
        r->update_height();
    }

    void right_rotate(TreeNode<K, V> *r)
    {
        TreeNode<K, V> *l = r->left, *p = r->fa;
        if (!l) return;
        r->left = l->right;
        l->right = r;

        l->fa = p;
        r->fa = l;
        if (p) {
            if (p->left == r) p->left = l;
            else p->right = l;
        } else {
            root = l;
        }
        if (r->left) r->left->fa = r;

        l->update_height();
        r->update_height();
    }

    void balance_fixup(TreeNode<K, V> *node)
    {
        if (!node) return;
        node->update_height();

        int factor = node->balance_factor();

        if (factor > 1) {
            if (node->left && node->left->balance_factor() < 0) left_rotate(node->left);
            right_rotate(node);
        } else if (factor < -1) {
            if (node->right && node->right->balance_factor() > 0) right_rotate(node->right);
            left_rotate(node);
        }
    }

    void balance_fixup_to_root(TreeNode<K, V> *node)
    {
        if (!node) return;
        TreeNode<K, V> *p = node->fa;
        while (p) {
            balance_fixup(node);
            node = p;
            p = node->fa;
        }
    }

    void insert(const K &key, const V &value)
    {
        TreeNode<K, V> *node = root, *pos = NULL;
        while (node) {
            pos = node;
            if (key < node->key) node = node->left;
            else if (key > node->key) node = node->right;
            else {
                return;
            }
        }
        // 走到这里必然不等
        TreeNode<K, V> *new_node = new TreeNode<K, V>(key, value);
        new_node->fa = pos; // 先设置父亲
        if (!pos) root = new_node;
        else if (key < pos->key) pos->left = new_node;
        else pos->right = new_node;
        // 从pos开始维护平衡
        if (pos) balance_fixup_to_root(pos);
    }

    bool remove(const K &key)
    {
        TreeNode<K, V> *node = root, *p = NULL;
        while (node) {
            if (key != node->key) p = node;
            if (key < node->key) node = node->left;
            else if (key > node->key) node = node->right;
            else break;
        }
        if (!node) return false;
        // node就是要找的节点，以下分情况讨论
        // p1. 无孩子
        if (!node->left && !node->right) {
            // 直接删除。
            if (p) {
                if (p->left == node) p->left = NULL;
                else if (p->right == node) p->right = NULL;
            } else root = NULL;
            delete node;
            balance_fixup_to_root(p);
        }
        // p2. 只有一个孩子，用孩子替代它
        else if (!node->left) {
            TreeNode<K, V> *right = node->right;
            node->key = right->key;
            node->value = right->value;
            node->left = right->left;
            node->right = right->right;
            if (node->left) node->left->fa = node;
            if (node->right) node->right->fa = node;
            delete right;
            balance_fixup_to_root(node);
        } else if (!node->right) {
            TreeNode<K, V> *left = node->left;
            node->key = left->key;
            node->value = left->value;
            node->left = left->left;
            node->right = left->right;
            if (node->left) node->left->fa = node;
            if (node->right) node->right->fa = node;
            delete left;
            balance_fixup_to_root(node);
        }
        // p3. 有两个孩子，用后继替代它
        else {
            TreeNode<K, V> *succ = node->right, *succ_p = node;
            while (succ->left) succ_p = succ, succ = succ->left;
            node->key = succ->key;
            node->value = succ->value;

            if (succ == succ_p->left) succ_p->left = succ->right;
            else succ_p->right = succ->right;

            if (succ->right) succ->right->fa = succ_p;
            delete succ;
            balance_fixup_to_root(succ_p);
        }
        return true;
    }

    TreeNode<K, V> *search(const K &key) const {
        TreeNode<K, V> *node = root;
        while (node) {
            if (node->key < key) node = node->right;
            else if (node->key > key) node = node->left;
            else return node;
        }
        return NULL;
    }

    TreeNode<K, V> *minimum(TreeNode<K, V> *node) const {
        if (!node) return nullptr;
        while (node->left) node = node->left;
        return node;
    }

    TreeNode<K, V> *maximum(TreeNode<K, V> *node) const {
        if (!node) return nullptr;
        while (node->right) node = node->right;
        return node;
    }

    TreeNode<K, V> *predecessor(TreeNode<K, V> *node) const {
        if (!node) return nullptr;
        TreeNode<K, V> *left = node->left;
        if (left) return maximum(left);
        while (node->fa && node->fa->left == node) node = node->fa;
        return node->fa;
    }

    TreeNode<K, V> *successor(TreeNode<K, V> *node) const {
        if (!node) return nullptr;
        TreeNode<K, V> *right = node->right;
        if (right) return minimum(right);
        while (node->fa && node->fa->right == node) node = node->fa;
        return node->fa; 
    }

    struct Inter {
        TreeMap<K, V> *map;
        K key;
        Inter(TreeMap<K, V> *map, K key) : map(map), key(key) {}
        ~Inter() {}
        operator V() {
            TreeNode<K, V> *node = map->search(key);
            return node->value; // 我当然知道 map->search 可能返回 NULL，这是故意的，这样访问不存在的key会直接segfault
        }

        V operator=(const V &other) {
            TreeNode<K, V> *val = map->search(key);
            if (!val) map->insert(key, other);
            else val->value = other;
            return other;
        }

        V operator=(const Inter &other) {
            K key = other.key;
            V value = map->search(key)->value;
            return operator=(value);
        }
    };
public:
    TreeMap() {}
    TreeMap operator=(const TreeMap &other) {
        root = new TreeNode<K, V>(other.root);
        return *this;
    }
    TreeMap(const TreeMap &other) {
        root = new TreeNode<K, V>(other.root);
    }
    ~TreeMap() {
        delete root;
    }
    Inter operator[](const K &key) {
        return Inter(this, key);
    }

    bool erase(const K &key) {
        return remove(key);
    }

    int count(const K &key) const {
        return search(key) != NULL;
    }

    void update(const TreeMap &other) {
        for (Pair<K, V> p: other) {
            (*this)[p.first] = p.second;
        }
    }

    friend class Iterator;
    class Iterator {
    private:
        const TreeMap<K, V> *parent;
        TreeNode<K, V> *current_node;
    public:
        Iterator(const TreeMap<K, V> *parent, TreeNode<K, V> *current_node)
          : parent(parent), current_node(current_node) {}
        
        Pair<K, V> operator*() {
            return Pair<K, V>(current_node->key, current_node->value);
        }
        Iterator &operator++() {
            current_node = parent->successor(current_node);
            return *this;
        }
        Iterator operator++(int) {
            Iterator temp = *this;
            current_node = parent->successor(current_node);
            return temp;
        }
        Iterator &operator--() {
            current_node = parent->predecessor(current_node);
            return *this;
        }
        Iterator operator--(int) {
            Iterator temp = *this;
            current_node = parent->predecessor(current_node);
            return temp;
        }
        bool operator==(const Iterator &other) {
            return current_node == other.current_node;
        }
        bool operator!=(const Iterator &other) {
            return current_node != other.current_node;
        }
    };

    Iterator begin() const { return Iterator(this, minimum(root)); }
    Iterator end() const { return Iterator(this, nullptr); }

    void output() {
        io.print('{');
        TreeNode<K, V> *node = minimum(root), *succ = successor(node);
        while (node) {
            io.print(node->key, ": ", node->value);
            if (succ) io.print(", ");
            node = succ;
            succ = successor(succ);
        }
        io.print("}");
    }
};
