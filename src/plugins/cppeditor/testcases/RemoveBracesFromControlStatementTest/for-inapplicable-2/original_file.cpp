void foo(int v)
{
    @for (int i = 0; i < v; ++i) {
        if (i < v/2)
            ++i;
    }
}
