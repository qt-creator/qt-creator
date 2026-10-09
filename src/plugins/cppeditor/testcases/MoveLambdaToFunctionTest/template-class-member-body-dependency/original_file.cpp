struct QObject
{
    static void connect(...);
};

template<typename T> void use(T);

template<typename T>
struct Foo : QObject
{
    void setup()
    {
        connect(nullptr, nullptr, [this] { us@e(T()); });
    }
};
