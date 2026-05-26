struct S { QList<int> list; };

void f()
{
    S s;
    for (int x : std::as_const(s.list)) {}
}
