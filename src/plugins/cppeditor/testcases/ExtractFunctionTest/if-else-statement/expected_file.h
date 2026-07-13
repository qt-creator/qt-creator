inline void extracted(int dummy)
{
    if (dummy < 10)
        ++dummy;
    else
        --dummy;
}

inline void func()
{
    int dummy = 0;
    extracted(dummy);
}
