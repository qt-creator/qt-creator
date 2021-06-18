struct S {
    void memberFunc();
};

void func()
{
    const auto p = &S::mem /* COMPLETE HERE */;
}
