QList<int> getList();

void f()
{
    for (int x : @getList()) {}
}
