void f()
{
    QString s;
    for (QChar c : std::as_const(s)) {}
}
