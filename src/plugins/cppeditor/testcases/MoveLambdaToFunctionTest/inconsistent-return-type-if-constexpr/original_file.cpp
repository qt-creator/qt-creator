struct QObject
{
    static void connect(...);
};

void setup()
{
    QObject::connect(nullptr, nullptr, [] {
        if constexpr (false)
            return 1;
        return 2.@0;
    });
}
