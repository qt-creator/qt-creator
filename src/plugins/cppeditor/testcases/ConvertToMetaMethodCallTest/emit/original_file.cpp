class C {
public:
    C() {
        emit this->@aSignal();
    }

signals:
    void aSignal();
};
