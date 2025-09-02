struct S {
    template<typename In>
    void foo(In in);
};

template<typename In>
void S::foo(In in) { (void)in; }
