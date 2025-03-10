struct S {

};
class C {
public:
    void setValue(int v) { m_s.@setValue(v); }
private:
    static S m_s;
};
