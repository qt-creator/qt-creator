class Foo
{
public:
    void extracted();

private:
    void bar();
};
inline void Foo::extracted()
{
    g();
}

void Foo::bar()
{
    extracted();
}
