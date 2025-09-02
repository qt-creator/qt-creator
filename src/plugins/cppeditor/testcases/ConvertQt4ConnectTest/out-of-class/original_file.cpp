class QObject {};
class TestClass : public QObject
{
public:
    void setProp(int) {}
    void sigFoo(int) {}
};

int foo()
{
    TestClass obj;
    conne@ct(&obj, SIGNAL(sigFoo(int)), &obj, SLOT(setProp(int)));
}
