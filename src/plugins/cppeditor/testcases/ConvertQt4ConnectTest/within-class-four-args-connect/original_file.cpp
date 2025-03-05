template<class T>
struct Pointer
{
    T *t;
    operator T*() const { return t; }
    T *data() const { return t; }
};
class QObject {};
class TestClass : public QObject
{
public:
    void setProp(int) {}
    void sigFoo(int) {}
    void setupSignals();
};

void TestClass::setupSignals()
{
    conne@ct(this, SIGNAL(sigFoo(int)), this, SLOT(setProp(int)));
}
