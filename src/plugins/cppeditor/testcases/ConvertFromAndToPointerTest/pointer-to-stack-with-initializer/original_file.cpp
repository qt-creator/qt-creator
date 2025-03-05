void foo() {
    QString *@str = new QString(QLatin1String("schnurz"));
    if (!str->isEmpty())
        str->clear();
}
