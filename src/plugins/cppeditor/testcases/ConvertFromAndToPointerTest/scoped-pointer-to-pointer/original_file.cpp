template<typename T> class QScopedPointer
{
public:
    QScopedPointer(T *p) {}
    T *operator->() const;
    T &operator*() const;
    T *get() const;
    void reset(T *p = nullptr);
};

class Foo
{
public:
    Foo(int, int) {}
    void bar();
};

void func()
{
    QScopedPointer<Foo> @f(new Foo(1, 2));
    f->bar();
    f.reset();
}
