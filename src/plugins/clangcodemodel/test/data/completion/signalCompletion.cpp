class QObject {
public:
    void aSignal() __attribute__((annotate("qt_signal")));
    void anotherSignal() __attribute__((annotate("qt_signal")));
    void notASignal();
    static void connect();
    static void disconnect();
};
class DerivedFromQObject : public QObject {
public:
    void myOwnSignal() __attribute__((annotate("qt_signal")));
    void alsoNotASignal();
};
class NotAQObject {
public:
    void notASignal();
    void alsoNotASignal();
    static void connect();
};

void someFunction()
{
 /* COMPLETE HERE */
