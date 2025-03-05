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
        connect(NsA::ClassA::instance(), &NsA::ClassA::sig, this, &ClassB::slot);
    }
};
}
