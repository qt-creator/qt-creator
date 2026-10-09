struct QObject
{
    static void connect(...);
};

void movedLambda() noexcept
{
    doStuff();
}

void setup()
{
    QObject::connect(nullptr, nullptr, movedLambda);
}
