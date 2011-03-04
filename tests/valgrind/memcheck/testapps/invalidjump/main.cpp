void foo() { }

int main()
{
    void (*fooPtr)() = 0;
    fooPtr();
    return 0;
}
