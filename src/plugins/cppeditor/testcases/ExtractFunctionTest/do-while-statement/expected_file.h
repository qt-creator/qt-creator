inline void extracted(int dummy)
{
    do
        ++dummy;
    while (dummy < 10);
}

inline void func()
{
    int dummy = 0;
    extracted(dummy);
}
