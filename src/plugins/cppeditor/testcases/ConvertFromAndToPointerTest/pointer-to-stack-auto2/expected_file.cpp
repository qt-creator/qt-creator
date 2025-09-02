void foo() {
    auto str = QString();
    if (!str.isEmpty())
        str.clear();
    f1(str);
    f2(&str);
}
