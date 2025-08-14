class C1 {};
class C2 {};
static void func() {}
class C3 {
    friend void func();
    friend void func2() {}
    friend class C1;
    friend C2;
};
