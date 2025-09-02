class C {
public:
    C() {
        C c;
        this->@noArgs();
    }

private:
    Q_SIGNAL void noArgs();
};
