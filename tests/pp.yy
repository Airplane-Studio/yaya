%define M7() 1
var M7 = 5;
print(M7, M7());
%undef M7
%define M7 ()
func ret3() { return 3; }
print(ret3 M7);