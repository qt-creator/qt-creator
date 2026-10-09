struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    struct Nested {};
    void setup();

private:
    Nested movedLambda(Nested n);};

void Foo::setup()
{
    connect(nullptr, nullptr, this, &Foo::movedLambda);
}

Foo::Nested Foo::movedLambda(Nested n)
{
    return doStuff(n);
}
