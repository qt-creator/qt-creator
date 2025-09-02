class C {
public:
    C() {
        this->@notInvokable();
    }

private:
    void notInvokable();
};
