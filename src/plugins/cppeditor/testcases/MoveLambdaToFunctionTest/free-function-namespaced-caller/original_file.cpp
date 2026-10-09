struct QObject
{
    static void connect(...);
};

namespace ns {
struct Bar {};
void setup();
}

void ns::setup()
{
    QObject::connect(nullptr, nullptr, [](Bar b) -> Bar { return doSt@uff(b); });
}
