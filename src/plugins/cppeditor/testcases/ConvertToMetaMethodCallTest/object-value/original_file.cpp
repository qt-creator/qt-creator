class C {
public:
    C() {
        C c;
        c.@aSignal();
    }

signals:
    void aSignal();
};
