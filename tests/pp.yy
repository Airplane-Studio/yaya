%define PP_CONCAT_IMPL(a, b) a$$b
%define PP_CONCAT(a, b) PP_CONCAT_IMPL(a, b)
%define bar() baz

PP_CONCAT(foo, bar())
PP_CONCAT_IMPL(foo, bar())