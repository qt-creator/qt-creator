auto func()
{
    return R"(foo
    foobar
        R"notaprefix!(
    barfoobar)" R"(second)" /* comment */ R"(third)";
}

void keywords()
{
    bool b1 = true;
    bool b2 = false;
    void *p = nullptr;
}

void numberLiterals()
{
    auto integer         = 1;
    auto numFloat1       = 1.2f;
    auto numFloat2       = 1.2;
}

template<int n = 5> class C;

struct ConversionFunction {
    operator int();
};
