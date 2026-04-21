%define MUL3(a, b) MUL2(a, b)
%define MUL2(a, b) a * b
%define MUL(a, b) MUL3(a, b)
%define THREE 1 + 2
%define FIVE 2 + 3
print(MUL(THREE, FIVE));