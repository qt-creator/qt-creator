struct S {

    int value;
};
class C {
public:
    void setValue(int v) { m_s.value = v; }
private:
    S m_s;
};
