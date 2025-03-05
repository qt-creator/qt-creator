namespace NS {
class C {
    void f(C &c);
};
}
void NS::C::f(NS::C &c)
{
    @{start}C *c2 = &c;@{end}
}
