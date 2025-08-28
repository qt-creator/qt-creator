int func() { return 0; }
class C {
public:
    C() : @m_x(func()) {}
private:
    int m_y;
};
