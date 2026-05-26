struct S { static QList<int> list; };

void f()
{
    for (int x : std::as_const(S::list)) {}
}
