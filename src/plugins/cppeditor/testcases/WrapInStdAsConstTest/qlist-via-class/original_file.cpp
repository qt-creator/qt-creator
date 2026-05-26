struct S { static QList<int> list; };

void f()
{
    for (int x : @S::list) {}
}
