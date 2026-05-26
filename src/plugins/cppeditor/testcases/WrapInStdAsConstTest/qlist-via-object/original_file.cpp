struct S { QList<int> list; };

void f()
{
    S s;
    for (int x : @s.list) {}
}
