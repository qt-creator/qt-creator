#include <QMetaObject>

class C {
public:
    C() {
        C c;
        QMetaObject::invokeMethod(this, "noArgs");
    }

private:
    Q_SIGNAL void noArgs();
};
