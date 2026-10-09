struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    void setup();

private:
    void movedLambda();};

void Foo::setup()
{
    connect(nullptr, nullptr, this, &Foo::movedLambda, 1);
}

void Foo::movedLambda()
{
    doStuff();
}
