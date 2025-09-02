struct S {

    void setValue(int);
};
class C {
public:
    void setValue(int v) { m_s.setValue(v); }
private:
    S m_s;
};
