struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    void setup();

private:
    int movedLambda();};

void Foo::setup()
{
    connect(nullptr, nullptr, this, &Foo::movedLambda);
}

int Foo::movedLambda()
{
    return doStuff();
}
