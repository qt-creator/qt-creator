struct QObject
{
    static void connect(...);
};

struct Widget : QObject
{
};

struct Foo : QObject
{
    Widget *w;
    void setup();
};

void Foo::setup()
{
    connect(nullptr, nullptr, w, []{ doSt@uff(); });
}
