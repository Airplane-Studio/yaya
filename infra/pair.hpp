#ifndef _INFRA_PAIR_HPP
#define _INFRA_PAIR_HPP

#include "printer.hpp"

template <typename K, typename V>
class Pair {
public:
    K first;
    V second;
    Pair(K first, V second) : first(first), second(second) {}
    void output() {
        io.print("Pair(first = ", first, ", second = ", second, ")");
    }
};

#endif