void foo() {
    QString narf;
    QString *str = &narf;
    if (!str->isEmpty())
        str->clear();
    f1(*str);
    f2(str);
}
