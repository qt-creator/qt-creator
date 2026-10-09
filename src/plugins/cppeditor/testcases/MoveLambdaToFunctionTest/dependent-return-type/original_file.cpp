struct QObject
{
    static void connect(...);
};

template<typename T> T helper();

template<typename T>
struct Foo : QObject
{
    void setup()
    {
        connect(nullptr, nullptr, [this] { return hel@per<T>(); });
    }
};
