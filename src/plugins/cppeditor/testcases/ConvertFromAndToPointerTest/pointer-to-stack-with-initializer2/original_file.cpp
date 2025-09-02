void foo() {
    QString *@str = new QString;
    if (!str->isEmpty())
        str->clear();
}
