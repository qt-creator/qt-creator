struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    void setup();
};

void Foo::setup()
{
    connect(nullptr, nullptr, this, []() -> int { return doSt@uff(); });
}
