class Base
{
public:
    virtual int helper(int a, int b) { return a + b; }
};

int user(Base &b)
{
    return b.hel@per(1, 2);
}
