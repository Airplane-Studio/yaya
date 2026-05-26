%macro foo 1
    bar %1 * 3
    print("foo expanding");
%endmacro

%macro bar 1
    foo %1 + 2
    print("bar expanding");
%endmacro

foo 3