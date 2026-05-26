void f()
{
    QList<int> list;
    for (int x : std::as_const(list)) {}
}
