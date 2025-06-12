void foo(std::vector<int> &list)
{
    @for (int &i : list) {
        i *= 2;
    }
}
