struct S {

    static void setValue(int);
};
class C {
public:
    void setValue(int v) { m_s.setValue(v); }
private:
    static S m_s;
};
