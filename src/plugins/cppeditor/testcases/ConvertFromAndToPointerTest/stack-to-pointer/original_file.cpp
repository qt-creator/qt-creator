void foo() {
    QString @str;
    if (!str.isEmpty())
        str.clear();
    f1(str);
    f2(&str);
}
