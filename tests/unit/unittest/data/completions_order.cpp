class Constructor {
public:
    Constructor() = default;
    Constructor(int) {}
};

void testConstructor() {

}

class Base {
    virtual void bar(int a) const;
};

class DifferentPriorities : public Base {
public:
    void foo();
    void foo() const;
    void bar(int a) const override;
    void testBar() {

    }
};

void testPriorities() {
    DifferentPriorities d;
    d.
}

class LexicographicalSorting
{
public:
    void memberFuncBB();
    void memberFuncC();
    void memberFuncAAA() const;
};

void testLexicographicalSorting() {
    LexicographicalSorting ls;
    ls.memberFunc
}
