class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> @f = nullptr;
}
