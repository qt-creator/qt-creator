inline void extracted(int dummy)
{
    if (dummy < 10) {
        ++dummy;
    }
}

inline void func()
{
    int dummy = 0;
    extracted(dummy);
}
