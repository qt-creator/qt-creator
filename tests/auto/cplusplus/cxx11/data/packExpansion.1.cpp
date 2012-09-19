template <class ... Args>
int foo(Args args...) {
    bar(args..., {args...}, e, f);
}
