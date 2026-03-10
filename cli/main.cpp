#include "infra.h"

int main(int argc, const char *argv[])
{
    DynamicArray<int> dyn;
    for (int i = 0; i < 100; i++) {
        dyn.append(i);
    }
    TreeMap<int, int> map;
    for (int i = 0; i < 100; i++) {
        map[i] = i;
    }
    io.println(dyn);
    io.println(map);
    for (auto p: map) {
        io.println(p);
    }
    io.println("The number of argument given is ", argc - 1, " and they are: ");
    for (int i = 1; i < argc; i++) {
        io.println(argv[i]);
    }
    return 0;
}