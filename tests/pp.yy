%define va_arg_macro(a, b, ...) print(a + b, __VA_ARGS__)

va_arg_macro(1, 3);