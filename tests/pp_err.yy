%if this should be skipped
%if this should also be skipped
%else
print("test")
%endif
%elifdef I don't care
print("fuck")
%endif

%define Hello "Hello"
%define World "World"
Hello World "!"