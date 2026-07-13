inline void func()
{
    int dummy = 0;
    @{start}do@{end}
        ++dummy;
    while (dummy < 10);
}
