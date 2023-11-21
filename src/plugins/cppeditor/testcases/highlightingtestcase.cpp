auto func()
{
    return R"(foo

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

template<typename T> concept NoConstraint = true;

const char16_t *operator ""_w(const char16_t *s, size_t) { return s; }
const auto s = u"one"_w;
const auto s2 = L"hello";
const auto s3 = u8"hello";
const auto s4 = U"hello";
const auto s5 = uR"("o
     ne")"_w;
const auto s6 = u"o\
ne"_w;

static void parenTest()
{
    do {
        /* comment */ \
    } while (false);
}

const char* s7 = R"(
))";
