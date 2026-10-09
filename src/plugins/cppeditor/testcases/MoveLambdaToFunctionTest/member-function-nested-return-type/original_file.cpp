struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    struct Nested {};
    void setup();
};

void Foo::setup()
{
    connect(nullptr, nullptr, [this](Nested n) -> Nested { return doSt@uff(n); });
}
