class Foo {@
    Foo() = default;
    ~Foo() = default;
    Foo &operator=(const Foo &) = default;
    Foo &operator=(Foo &&) = delete;

    int numberA() const
    {
        return 5;
    }
    int numberB() const
    {
        return 5;
    }
};
