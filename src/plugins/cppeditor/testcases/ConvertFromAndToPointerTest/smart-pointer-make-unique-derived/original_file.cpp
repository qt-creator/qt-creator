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
    std::unique_ptr<Base> @f = std::make_unique<Derived>(1);
}
