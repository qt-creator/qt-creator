class C {
public:
    C() {
        (new C)->@aSignal();
    }

signals:
    void aSignal();
};
