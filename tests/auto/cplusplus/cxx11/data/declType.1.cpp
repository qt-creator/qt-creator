template <class T, class R>
auto foo(T t, R r) -> decltype(t + r)
{}

int x;
decltype(x) foo;
decltype(x) foo();

// this does not work yet, as decltype is only parsed as a simple-specifier
// and not also as a nested-name-specifier
//decltype(vec)::value_type a;
