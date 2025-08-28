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
    connect(&obj, &TestClass::sigFoo, &obj, &TestClass::setProp);
}
