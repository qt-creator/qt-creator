struct QObject
{
    static void connect(...);
};

void setup()
{
    QObject::connect(nullptr, nullptr, []{ doSt@uff(); });
}
