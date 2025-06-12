void foo(int v)
{
    @do {
        ++v;
        v
            *= 2;
    } while (v < 100);
}
