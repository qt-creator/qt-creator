void foo() {
    QString *str = new QString;
    str->clear();
    {
        QString str;
        str.clear();
    }
    f1(*str);
}
