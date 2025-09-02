#include <QMetaObject>

class C {
public:
    C() {
        C c;
        QMetaObject::invokeMethod(&c, "aSignal");
    }

signals:
    void aSignal();
};
