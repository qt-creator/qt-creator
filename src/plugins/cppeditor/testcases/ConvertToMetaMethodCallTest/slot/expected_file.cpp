#include <QMetaObject>

class C {
public:
    C() {
        QMetaObject::invokeMethod(this, "aSlot");
    }

private slots:
    void aSlot();
};
