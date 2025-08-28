#define CONST const
#define VOLATILE volatile
class Foo
{
    int func(int a, int b) CONST VOLATILE;
};

int Foo::func(int a, int b) const volatile
{
    return 42;
}
