struct QObject
{
    static void connect(...);
};

template<typename T> void use(T);

template<typename T>
void setup()
{
    QObject::connect(nullptr, nullptr, [] { us@e(T()); });
}
