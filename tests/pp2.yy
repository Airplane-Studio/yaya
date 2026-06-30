%define PP_DEFER(A) A PP_EMPTY()
# %define PP_EXPAND(...) __VA_ARGS__
%define PP_EMPTY()

%define A() 123
A()
PP_DEFER(A)()
# PP_EXPAND(PP_DEFER(A)())