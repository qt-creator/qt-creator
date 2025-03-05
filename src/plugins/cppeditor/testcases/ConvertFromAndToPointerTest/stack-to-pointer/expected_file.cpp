void foo() {
    QString *str = new QString;
    if (!str->isEmpty())
        str->clear();
    f1(*str);
    f2(str);
}
