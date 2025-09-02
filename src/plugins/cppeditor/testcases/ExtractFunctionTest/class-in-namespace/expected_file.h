namespace NS {
class C {
    void f(C &c);

public:
    void extracted(NS::C &c);
};
}
inline void NS::C::extracted(NS::C &c)
{
    C *c2 = &c;
}

void NS::C::f(NS::C &c)
{
    extracted(c);
}
