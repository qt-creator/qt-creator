class C {
public:
    C() {
        this->@oneArg(0);
    }

private:
    Q_SLOT void oneArg(int index);
};
