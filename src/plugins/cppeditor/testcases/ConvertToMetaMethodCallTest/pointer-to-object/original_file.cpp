class C {
public:
    C() {
        this->@aSignal();
    }

signals:
    void aSignal();
};
