void foo(int &x)
{
    @if (x == 0) {
        x = 1;
    } else if (x < 0) {
        x = -x;
    } else
        --x;
}
