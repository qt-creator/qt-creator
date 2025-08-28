class C {
public:
    C() {
        Q_EMIT this->@aSignal();
    }

signals:
    void aSignal();
};
