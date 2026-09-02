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
    connect(nullptr, nullptr, this, &Foo::movedLambda);
}

void Foo::movedLambda()
{
    doStuff();
}
