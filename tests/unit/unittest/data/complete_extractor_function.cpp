void Function();
template<class T> void TemplateFunction();
void FunctionWithOptional(int x, char y = 1, int z = 5);
#define FunctionMacro(X, Y) X + Y

class base {
    void NotAccessibleFunction();
};
class Class : public base {
    void Method();
    void MethodWithParameters(int x = 30);
    __attribute__((annotate("qt_slot"))) void Slot();
    __attribute__((annotate("qt_signal"))) void Signal();
    __attribute__ ((deprecated)) void DeprecatedFunction();
    void NotAvailableFunction() = delete;

public:
    void function()
    {

    }
};
