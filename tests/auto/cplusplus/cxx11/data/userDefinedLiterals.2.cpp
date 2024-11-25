wchar_t operator ""_wc(const wchar_t c) { return c; }

int main()
{
    const auto c = L'c'_wc;
    return c;
}
