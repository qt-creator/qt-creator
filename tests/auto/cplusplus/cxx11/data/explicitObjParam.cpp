struct S {
    void f(this S& self);
    template <typename Self> void g(this Self&& self, int);
};
