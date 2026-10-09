class Base
{
public:
    Base(int) {}
};
class Derived : public Base
{
public:
    Derived(int i) : Base(i) {}
};

void func()
{
    Base *f = new Derived(1);
}
