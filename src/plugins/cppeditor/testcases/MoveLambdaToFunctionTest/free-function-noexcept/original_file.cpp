struct QObject
{
    static void connect(...);
};

void setup()
{
    QObject::connect(nullptr, nullptr, []() noexcept { doSt@uff(); });
}
