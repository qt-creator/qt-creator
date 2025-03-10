class C {
public:
    void setValue(int v);
private:
    int m_value;
};
void C::setValue(int v) { this->@m_value = v; }
