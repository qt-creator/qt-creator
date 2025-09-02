#include <QMetaObject>

class C {
public:
    C() {
        QMetaObject::invokeMethod(this, "aSignal");
    }

signals:
    void aSignal();
};
