class Foo {
    Foo();
    ~Foo();
    Foo &operator=(const Foo &);
    Foo &operator=(Foo &&) = delete;

    int numberA() const;
    int numberB() const;
};
