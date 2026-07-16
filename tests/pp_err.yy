%define something
%ifdef something
print("Hello everyone I think we should skip this");
%ifndef idontknowwhatisthis
print(111)
%endif
%else
print("should not be preserved");
%endif
print("at least %endif left nothing lol");