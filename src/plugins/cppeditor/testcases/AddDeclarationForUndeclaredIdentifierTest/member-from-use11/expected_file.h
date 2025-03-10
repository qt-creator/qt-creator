class C {
public:
    static void setValue(int v);
private:
    static int m_value;
};
void C::setValue(int v) { @m_value = v; }
