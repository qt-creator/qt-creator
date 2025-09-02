class C {
public:
    static int value() { int i = @valueInternal(); return i; }
private:
    static int valueInternal();
};
