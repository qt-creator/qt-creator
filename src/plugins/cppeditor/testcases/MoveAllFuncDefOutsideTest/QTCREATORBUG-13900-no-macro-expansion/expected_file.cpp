#define FAKE_Q_OBJECT int bar() {return 5;}
class Foo {
    FAKE_Q_OBJECT
    int f1();
};

int Foo::f1()
{
    return 1;
}
