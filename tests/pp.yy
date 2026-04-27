%define PP_CONCAT_IMPL(a, b) a$$b
%define PP_CONCAT(a, b) PP_CONCAT_IMPL(a, b)
%define bar() baz

%define dbl(x) M10(x) * 2
%define M10(x) dbl(x) + 3

dbl(2)

PP_CONCAT(foo, bar())
PP_CONCAT_IMPL(foo, bar())