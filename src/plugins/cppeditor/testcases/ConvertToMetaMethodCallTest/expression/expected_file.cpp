#include <QMetaObject>

class C {
public:
    C() {
        QMetaObject::invokeMethod((new C), "aSignal");
    }

signals:
    void aSignal();
};
