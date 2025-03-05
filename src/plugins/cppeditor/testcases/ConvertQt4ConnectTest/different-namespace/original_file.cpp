namespace NsA {
class ClassA : public QObject
{
    static ClassA *instance();
signals:
    void sig();
};
}

namespace NsB {
class ClassB : public QObject
{
    void slot();
    void connector() {
        co@nnect(NsA::ClassA::instance(), SIGNAL(sig()), this, SLOT(slot()));
    }
};
}
