#include <QMetaObject>

class C {
public:
    C() {
        QMetaObject::invokeMethod(this, "oneArg", Q_ARG(int, 0));
    }

private:
    Q_SLOT void oneArg(int index);
};
