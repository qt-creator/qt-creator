class C {
public:
    static int value();
private:
    static int valueInternal();
};
int C::value() { return valueInternal(); }
