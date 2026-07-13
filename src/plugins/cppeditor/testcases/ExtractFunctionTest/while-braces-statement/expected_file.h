inline void extracted(int dummy)
{
    while (dummy < 10) {
        ++dummy;
    }
}

inline void func()
{
    int dummy = 0;
    extracted(dummy);
}
