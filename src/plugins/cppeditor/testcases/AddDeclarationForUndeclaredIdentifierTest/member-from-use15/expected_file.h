class C {
public:
    void setValue(int v);
private:
    void setValueInternal(int);
};
void C::setValue(int v) { this->setValueInternal(v); }
