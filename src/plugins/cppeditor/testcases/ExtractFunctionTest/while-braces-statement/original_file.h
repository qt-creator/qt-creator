inline void func()
{
    int dummy = 0;
    @{start}while@{end} (dummy < 10) {
        ++dummy;
    }
}
