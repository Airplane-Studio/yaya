%macro foo 1
    %define SYS_exit 60
    print("Before foo expansion");
    bar %1 * 3
    print("foo expanding");
%endmacro

%macro bar 1
    foo %1 + 2
    print("bar expanding");
%endmacro

foo 3
print(SYS_exit);