#ifndef _INFRA_DYNLIST_HPP
#define _INFRA_DYNLIST_HPP

#include "std_wrapper.h"
#include "printer.hpp"

template <typename T>
class DynamicArray {
private:
    T *arr;
    int len, capacity_;

    void adjust_capacity(int new_len) {
        if (new_len >= capacity_) {
            T *new_arr = new T[capacity_ * 2];
            for (int i = 0; i < len; i++) new_arr[i] = arr[i];
            delete[] arr;
            arr = new_arr;
            capacity_ *= 2;
        }
    }
    void init_arr(int new_len) {
        if (new_len == 0) capacity_ = 2;
        else capacity_ = new_len;
        arr = new T[capacity_];
        len = new_len;
    }
public:
    DynamicArray() {
        init_arr(0);
    }
    explicit DynamicArray(int len) {
        init_arr(len);
    }
    DynamicArray(T *buf, int len) : len(len), capacity_(len) {
        init_arr(len);
        for (int i = 0; i < len; i++) append(buf[i]);
    }
    DynamicArray(const DynamicArray<T> &other) {
        init_arr(other.len);
        for (int i = 0; i < len; i++) arr[i] = other.arr[i];
    }
    DynamicArray(DynamicArray<T> &&other) {
        arr = other.arr;
        len = other.len;
        capacity_ = other.capacity_;
        other.arr = nullptr;
    }
    DynamicArray &operator=(const DynamicArray<T> &other) {
        free(arr);
        init_arr(other.len);
        for (int i = 0; i < len; i++) arr[i] = other.arr[i];
        return *this;
    }
    DynamicArray &operator=(DynamicArray<T> &&other) {
        free(arr);
        arr = other.arr;
        len = other.len;
        capacity_ = other.capacity_;
        other.arr = nullptr;
        return *this;
    }
    ~DynamicArray() { delete[] arr; }
    void append(const T &value) {
        adjust_capacity(len + 1);
        arr[len++] = value;
    }
    void insert(int index, const T &value) {
        adjust_capacity(len + 1);
        for (int i = len - 1; i >= index; i++) arr[i + 1] = (T&&) arr[i];
        arr[index] = value;
        len++;
    }
    void remove(int index) {
        for (int i = index; i < len - 1; i++) arr[i] = (T&&) arr[i + 1];
        adjust_capacity(len - 1);
        len--;
    }
    void extend(const DynamicArray<T> &other) {
        for (int i = 0; i < other.size(); i++) append(other[i]);
    }
    bool empty() const { return !len; }
    void clear() {
        delete[] arr;
        init_arr(0);
    }
    void reverse() {
        for (int i = 0; i < len / 2; i++) {
            T temp = arr[i];
            arr[i] = arr[len - 1 - i];
            arr[len - 1 - i] = temp;
        }
    }
    T &operator[](int index) {
        return arr[index];
    }
    const T &operator[](int index) const {
        return arr[index];
    }
    DynamicArray<T> operator+(const DynamicArray<T> &other) {
        DynamicArray<T> res = *this;
        for (int i = 0; i < other.size(); i++) res.append(other[i]);
        return res;
    }
    DynamicArray<T> operator+=(const DynamicArray<T> &other) {
        for (int i = 0; i < other.size(); i++) append(other[i]);
        return *this;
    }
    bool contains(const T &other) const {
        for (int i = 0; i < len; i++) {
            if (arr[i] == other) return true;
        }
        return false;
    }
    int size() const { return len; }
    int capacity() const { return capacity_; }
    void output() {
        io.print('{');
        for (int i = 0; i < len; i++) {
            io.print(arr[i]);
            if (i != len - 1) io.print(", ");
        }
        io.print('}');
    }

    T *begin() { return arr; }
    T *end() { return arr + len; }
};

#endif