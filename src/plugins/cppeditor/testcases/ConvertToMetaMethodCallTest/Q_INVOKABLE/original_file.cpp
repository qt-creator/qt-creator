class C {
public:
    C() {
        C c;
        this->@twoArgs(0, c);
    }

private:
    Q_INVOKABLE void twoArgs(int index, const C &value);
};
