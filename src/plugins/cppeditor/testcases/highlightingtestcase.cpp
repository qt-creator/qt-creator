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
