class Something
{
    int mFoo;

public:
    int foo() const;
    void setFoo(int foo);
};

int Something::foo() const
{
    return mFoo;
}

void Something::setFoo(int foo)
{
    mFoo = foo;
}
