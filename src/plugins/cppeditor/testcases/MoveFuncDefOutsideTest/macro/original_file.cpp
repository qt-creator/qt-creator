#define CONST const
#define VOLATILE volatile
class Foo
{
    int fu@nc(int a, int b) CONST VOLATILE
    {
        return 42;
    }
};
