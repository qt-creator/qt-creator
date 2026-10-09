struct QObject
{
    static void connect(...);
};

namespace ns {
struct Bar {};
void setup();
}

namespace ns {
Bar movedLambda(Bar b)
{
    return doStuff(b);
}

}
void ns::setup()
{
    QObject::connect(nullptr, nullptr, movedLambda);
}
