struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    void setup()
    {
        connect(nullptr, nullptr, this, &Foo::movedLambda);
    }

private:
    void movedLambda();};

void Foo::movedLambda()
{
    doStuff();
}
