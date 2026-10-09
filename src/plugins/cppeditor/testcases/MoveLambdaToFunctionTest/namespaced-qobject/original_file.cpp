namespace QtNamespace {
struct QObject
{
    static void connect(...);
};
}

struct Foo : QtNamespace::QObject
{
    void setup();
};

void Foo::setup()
{
    connect(nullptr, nullptr, [this]{ doSt@uff(); });
}
