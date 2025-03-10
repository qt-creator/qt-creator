class C {
public:
    void setValue(int v);
};
void C::setValue(int v) { this->@setValueInternal(v); }
