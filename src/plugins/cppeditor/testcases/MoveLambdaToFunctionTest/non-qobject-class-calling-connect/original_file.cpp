struct QObject
{
    static void connect(...);
};

struct Foo
{
    void setup();
};

void Foo::setup()
{
    QObject::connect(nullptr, nullptr, [this]{ doSt@uff(); });
}
