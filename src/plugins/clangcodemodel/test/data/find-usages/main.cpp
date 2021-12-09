#include "defs.h"

int main()
{
    S2 s;
    auto *p = &s.value;
    int **pp;
    p = &s.value;
    *pp = &s.value;
    p = &S::value;
    s.p = &s.value;
    S::p = &s.value;
    (&s)->p = &((new S2)->value);
    const int *p2 = &s.value;
    s.p2 = &s.value;
    int * const p3 = &s.value;
    int &r = s.value;
    const int &cr = s.value;
    func1(s.value);
    func2(s.value);
    func3(&s.value);
    func4(&s.value);
    func5(s.value);
    *p = 5;
    func1(*p);
    func2(*p);
    func3(p);
    func4(p);
    func5(*p);
    int &r2 = *p;
    const int &cr2 = *p;
    s = S2();
    auto * const ps = &s;
    const auto *ps2 = &s;
    auto &pr = s;
    const auto pr2 = &s;
    s.constFunc().nonConstFunc();
    s.nonConstFunc();
    (&s)->nonConstFunc();
    s.n.constFunc();
    s.n.nonConstFunc();
    s.n.constFunc(s.value);
    delete s.p;
    switch (S::value) {
        case S::value3: break;
    }
    switch (S::value = 5) {
        default: break;
    }
    if (S::value) {}
    if (S::value = 0) {}
    ++S::value;
    S::value--;
    s.staticFunc1();
    s.staticFunc2();
    S::value = sizeof S::value;
    int array[3];
    array[S::value] = S::value;
    S::value = array[S::value];
    const auto funcPtr = &func1;
}
