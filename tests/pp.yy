var M41 = 2;
var M5 = 6;
%define M41 M5 + 2
%define M5 M41 * 3
print(M41 + M5,"Hello M41 M5", M5 * M41);