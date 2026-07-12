%define print_err(fmt, args...) fprintf(stderr, fmt __VA_OPT__(,) args)
print_err("Hello");
print_err("Hello", 1);
print_err("Hello", );

%warning this a warning
# %error this is an error