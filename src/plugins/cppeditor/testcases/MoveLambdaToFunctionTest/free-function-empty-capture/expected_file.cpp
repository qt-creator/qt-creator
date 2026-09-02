struct QObject
{
    static void connect(...);
};

void movedLambda()
{
    doStuff();
}

void setup()
{
    QObject::connect(nullptr, nullptr, movedLambda);
}
