#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include "exceptions.hpp"
#include <cstddef>

namespace sjtu {

template <class T> class deque {
private:
    static const int MAX_BLOCK_SIZE = 500;
    static const int CAPACITY = MAX_BLOCK_SIZE + 1;

    struct Node {
        T* data;
        int head;
        int size;
        Node* next;
        Node* prev;

        Node() : head(0), size(0), next(nullptr), prev(nullptr) {
            data = reinterpret_cast<T*>(new char[CAPACITY * sizeof(T)]);
        }

        ~Node() {
            for (int i = 0; i < size; ++i) {
                get_ptr(i)->~T();
            }
            delete[] reinterpret_cast<char*>(data);
        }

        T* get_ptr(int i) {
            return reinterpret_cast<T*>(reinterpret_cast<char*>(data) + ((head + i) % CAPACITY) * sizeof(T));
        }

        const T* get_ptr(int i) const {
            return reinterpret_cast<const T*>(reinterpret_cast<const char*>(data) + ((head + i) % CAPACITY) * sizeof(T));
        }

        void insert(int pos, const T& val) {
            if (pos < size / 2) {
                head = (head - 1 + CAPACITY) % CAPACITY;
                if (pos > 0) {
                    new (get_ptr(0)) T(*get_ptr(1));
                    for (int i = 1; i < pos; ++i) {
                        *get_ptr(i) = *get_ptr(i + 1);
                    }
                    get_ptr(pos)->~T();
                }
                new (get_ptr(pos)) T(val);
            } else {
                if (pos < size) {
                    new (get_ptr(size)) T(*get_ptr(size - 1));
                    for (int i = size - 1; i > pos; --i) {
                        *get_ptr(i) = *get_ptr(i - 1);
                    }
                    get_ptr(pos)->~T();
                }
                new (get_ptr(pos)) T(val);
            }
            size++;
        }

        void erase(int pos) {
            if (pos < size / 2) {
                for (int i = pos; i > 0; --i) {
                    *get_ptr(i) = *get_ptr(i - 1);
                }
                get_ptr(0)->~T();
                head = (head + 1) % CAPACITY;
            } else {
                for (int i = pos; i < size - 1; ++i) {
                    *get_ptr(i) = *get_ptr(i + 1);
                }
                get_ptr(size - 1)->~T();
            }
            size--;
        }

        void push_back(const T& val) {
            new (get_ptr(size)) T(val);
            size++;
        }

        void push_front(const T& val) {
            head = (head - 1 + CAPACITY) % CAPACITY;
            new (get_ptr(0)) T(val);
            size++;
        }

        void pop_back() {
            get_ptr(size - 1)->~T();
            size--;
        }

        void pop_front() {
            get_ptr(0)->~T();
            head = (head + 1) % CAPACITY;
            size--;
        }
    };

    Node* head_node;
    Node* tail_node;
    size_t total_size;

    void split(Node* node) {
        Node* new_node = new Node();
        int half = node->size / 2;
        for (int i = half; i < node->size; ++i) {
            new_node->push_back(*node->get_ptr(i));
            node->get_ptr(i)->~T();
        }
        node->size = half;
        
        new_node->next = node->next;
        new_node->prev = node;
        if (node->next) node->next->prev = new_node;
        else tail_node = new_node;
        node->next = new_node;
    }

    void merge(Node* node, Node* next_node) {
        for (int i = 0; i < next_node->size; ++i) {
            node->push_back(*next_node->get_ptr(i));
        }
        node->next = next_node->next;
        if (next_node->next) next_node->next->prev = node;
        else tail_node = node;
        
        delete next_node;
    }

    void maintain_after_insert(Node* node) {
        if (node->size >= MAX_BLOCK_SIZE) {
            split(node);
        }
    }

    void maintain_after_erase(Node* node) {
        if (node->prev && node->prev->size + node->size <= MAX_BLOCK_SIZE * 0.75) {
            merge(node->prev, node);
        } else if (node->next && node->size + node->next->size <= MAX_BLOCK_SIZE * 0.75) {
            merge(node, node->next);
        } else if (node->size == 0) {
            if (node == head_node && node == tail_node) return;
            
            if (node->prev) node->prev->next = node->next;
            else head_node = node->next;
            
            if (node->next) node->next->prev = node->prev;
            else tail_node = node->prev;
            
            delete node;
        }
    }

public:
    class const_iterator;
    class iterator {
        friend class deque;
        friend class const_iterator;
    private:
        deque* deq;
        Node* node;
        int pos;

    public:
        iterator(deque* d = nullptr, Node* n = nullptr, int p = 0) : deq(d), node(n), pos(p) {}

        iterator operator+(const int &n) const {
            if (n < 0) return operator-(-n);
            iterator res = *this;
            res += n;
            return res;
        }
        iterator operator-(const int &n) const {
            if (n < 0) return operator+(-n);
            iterator res = *this;
            res -= n;
            return res;
        }

        int operator-(const iterator &rhs) const {
            if (deq != rhs.deq) throw invalid_iterator();
            int idx1 = 0;
            if (node == nullptr) {
                idx1 = deq->total_size;
            } else {
                Node* curr = deq->head_node;
                while (curr != node) {
                    idx1 += curr->size;
                    curr = curr->next;
                }
                idx1 += pos;
            }
            
            int idx2 = 0;
            if (rhs.node == nullptr) {
                idx2 = deq->total_size;
            } else {
                Node* curr = deq->head_node;
                while (curr != rhs.node) {
                    idx2 += curr->size;
                    curr = curr->next;
                }
                idx2 += rhs.pos;
            }
            return idx1 - idx2;
        }

        iterator &operator+=(const int &n) {
            if (n < 0) return operator-=(-n);
            int rem = n;
            while (node && pos + rem >= node->size) {
                rem -= (node->size - pos);
                node = node->next;
                pos = 0;
            }
            if (node) {
                pos += rem;
            } else {
                if (rem > 0) throw invalid_iterator();
            }
            return *this;
        }

        iterator &operator-=(const int &n) {
            if (n < 0) return operator+=(-n);
            int rem = n;
            if (node == nullptr) {
                if (rem == 0) return *this;
                node = deq->tail_node;
                pos = node->size;
            }
            while (node && pos - rem < 0) {
                rem -= pos;
                node = node->prev;
                if (node) pos = node->size;
            }
            if (node) {
                pos -= rem;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        iterator &operator++() {
            return operator+=(1);
        }
        iterator operator--(int) {
            iterator tmp = *this;
            --(*this);
            return tmp;
        }
        iterator &operator--() {
            return operator-=(1);
        }

        T &operator*() const {
            if (!node || pos >= node->size) throw invalid_iterator();
            return *node->get_ptr(pos);
        }
        T *operator->() const noexcept {
            return node->get_ptr(pos);
        }

        bool operator==(const iterator &rhs) const {
            return deq == rhs.deq && node == rhs.node && pos == rhs.pos;
        }
        bool operator==(const const_iterator &rhs) const {
            return deq == rhs.deq && node == rhs.node && pos == rhs.pos;
        }
        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    class const_iterator {
        friend class deque;
    private:
        const deque* deq;
        const Node* node;
        int pos;

    public:
        const_iterator(const deque* d = nullptr, const Node* n = nullptr, int p = 0) : deq(d), node(n), pos(p) {}
        const_iterator(const iterator& other) : deq(other.deq), node(other.node), pos(other.pos) {}

        const_iterator operator+(const int &n) const {
            if (n < 0) return operator-(-n);
            const_iterator res = *this;
            res += n;
            return res;
        }
        const_iterator operator-(const int &n) const {
            if (n < 0) return operator+(-n);
            const_iterator res = *this;
            res -= n;
            return res;
        }

        int operator-(const const_iterator &rhs) const {
            if (deq != rhs.deq) throw invalid_iterator();
            int idx1 = 0;
            if (node == nullptr) {
                idx1 = deq->total_size;
            } else {
                const Node* curr = deq->head_node;
                while (curr != node) {
                    idx1 += curr->size;
                    curr = curr->next;
                }
                idx1 += pos;
            }
            
            int idx2 = 0;
            if (rhs.node == nullptr) {
                idx2 = deq->total_size;
            } else {
                const Node* curr = deq->head_node;
                while (curr != rhs.node) {
                    idx2 += curr->size;
                    curr = curr->next;
                }
                idx2 += rhs.pos;
            }
            return idx1 - idx2;
        }

        const_iterator &operator+=(const int &n) {
            if (n < 0) return operator-=(-n);
            int rem = n;
            while (node && pos + rem >= node->size) {
                rem -= (node->size - pos);
                node = node->next;
                pos = 0;
            }
            if (node) {
                pos += rem;
            } else {
                if (rem > 0) throw invalid_iterator();
            }
            return *this;
        }

        const_iterator &operator-=(const int &n) {
            if (n < 0) return operator+=(-n);
            int rem = n;
            if (node == nullptr) {
                if (rem == 0) return *this;
                node = deq->tail_node;
                pos = node->size;
            }
            while (node && pos - rem < 0) {
                rem -= pos;
                node = node->prev;
                if (node) pos = node->size;
            }
            if (node) {
                pos -= rem;
            } else {
                throw invalid_iterator();
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        const_iterator &operator++() {
            return operator+=(1);
        }
        const_iterator operator--(int) {
            const_iterator tmp = *this;
            --(*this);
            return tmp;
        }
        const_iterator &operator--() {
            return operator-=(1);
        }

        const T &operator*() const {
            if (!node || pos >= node->size) throw invalid_iterator();
            return *node->get_ptr(pos);
        }
        const T *operator->() const noexcept {
            return node->get_ptr(pos);
        }

        bool operator==(const iterator &rhs) const {
            return deq == rhs.deq && node == rhs.node && pos == rhs.pos;
        }
        bool operator==(const const_iterator &rhs) const {
            return deq == rhs.deq && node == rhs.node && pos == rhs.pos;
        }
        bool operator!=(const iterator &rhs) const {
            return !(*this == rhs);
        }
        bool operator!=(const const_iterator &rhs) const {
            return !(*this == rhs);
        }
    };

    deque() : head_node(new Node()), tail_node(head_node), total_size(0) {}

    deque(const deque &other) : head_node(new Node()), tail_node(head_node), total_size(0) {
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
            push_back(*it);
        }
    }

    ~deque() {
        clear();
        delete head_node;
    }

    deque &operator=(const deque &other) {
        if (this == &other) return *this;
        clear();
        for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
            push_back(*it);
        }
        return *this;
    }

    T &at(const size_t &pos) {
        if (pos >= total_size) throw index_out_of_bound();
        Node* curr = head_node;
        size_t p = pos;
        while (curr && p >= curr->size) {
            p -= curr->size;
            curr = curr->next;
        }
        return *curr->get_ptr(p);
    }

    const T &at(const size_t &pos) const {
        if (pos >= total_size) throw index_out_of_bound();
        const Node* curr = head_node;
        size_t p = pos;
        while (curr && p >= curr->size) {
            p -= curr->size;
            curr = curr->next;
        }
        return *curr->get_ptr(p);
    }

    T &operator[](const size_t &pos) {
        return at(pos);
    }

    const T &operator[](const size_t &pos) const {
        return at(pos);
    }

    const T &front() const {
        if (empty()) throw container_is_empty();
        return *head_node->get_ptr(0);
    }

    const T &back() const {
        if (empty()) throw container_is_empty();
        return *tail_node->get_ptr(tail_node->size - 1);
    }

    iterator begin() {
        if (empty()) return end();
        return iterator(this, head_node, 0);
    }

    const_iterator cbegin() const {
        if (empty()) return cend();
        return const_iterator(this, head_node, 0);
    }

    iterator end() {
        return iterator(this, nullptr, 0);
    }

    const_iterator cend() const {
        return const_iterator(this, nullptr, 0);
    }

    bool empty() const {
        return total_size == 0;
    }

    size_t size() const {
        return total_size;
    }

    void clear() {
        Node* curr = head_node;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head_node = new Node();
        tail_node = head_node;
        total_size = 0;
    }

    iterator insert(iterator pos, const T &value) {
        if (pos.deq != this) throw invalid_iterator();
        if (pos.node == nullptr) {
            push_back(value);
            return iterator(this, tail_node, tail_node->size - 1);
        }
        
        Node* node = pos.node;
        int p = pos.pos;
        node->insert(p, value);
        total_size++;
        
        maintain_after_insert(node);
        
        if (p < node->size) {
            return iterator(this, node, p);
        } else {
            return iterator(this, node->next, p - node->size);
        }
    }

    iterator erase(iterator pos) {
        if (pos.deq != this || pos.node == nullptr) throw invalid_iterator();
        if (empty()) throw container_is_empty();
        
        Node* node = pos.node;
        int p = pos.pos;
        
        int abs_idx = 0;
        Node* curr = head_node;
        while (curr != node) {
            abs_idx += curr->size;
            curr = curr->next;
        }
        abs_idx += p;
        
        node->erase(p);
        total_size--;
        
        maintain_after_erase(node);
        
        if (abs_idx == total_size) return end();
        
        curr = head_node;
        while (curr && abs_idx >= curr->size) {
            abs_idx -= curr->size;
            curr = curr->next;
        }
        return iterator(this, curr, abs_idx);
    }

    void push_back(const T &value) {
        if (tail_node->size >= MAX_BLOCK_SIZE) {
            Node* new_node = new Node();
            tail_node->next = new_node;
            new_node->prev = tail_node;
            tail_node = new_node;
        }
        tail_node->push_back(value);
        total_size++;
    }

    void pop_back() {
        if (empty()) throw container_is_empty();
        tail_node->pop_back();
        total_size--;
        maintain_after_erase(tail_node);
    }

    void push_front(const T &value) {
        if (head_node->size >= MAX_BLOCK_SIZE) {
            Node* new_node = new Node();
            new_node->next = head_node;
            head_node->prev = new_node;
            head_node = new_node;
        }
        head_node->push_front(value);
        total_size++;
    }

    void pop_front() {
        if (empty()) throw container_is_empty();
        head_node->pop_front();
        total_size--;
        maintain_after_erase(head_node);
    }
};

} // namespace sjtu

#endif