struct QObject
{
    static void connect(...);
};

struct Foo : QObject
{
    void setup()
    {
        connect(nullptr, nullptr, [this]{ doSt@uff(); });
    }
};
