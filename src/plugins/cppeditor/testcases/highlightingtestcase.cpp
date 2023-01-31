auto func()
{
    return R"(foo
    foobar
        R"notaprefix!(
    barfoobar)" R"(second)" /* comment */ R"(third)";
}
