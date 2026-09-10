class Foo
{
public:
    void bar();
};

void func()
{
    std::unique_ptr<Foo> @f(new Foo);
    f->bar();
    Foo *raw = f.release();
}
