#include <QMetaObject>

class C {
public:
    C() {
        C c;
        QMetaObject::invokeMethod(this, "twoArgs", Q_ARG(int, 0), Q_ARG(C, c));
    }

private:
    Q_INVOKABLE void twoArgs(int index, const C &value);
};
