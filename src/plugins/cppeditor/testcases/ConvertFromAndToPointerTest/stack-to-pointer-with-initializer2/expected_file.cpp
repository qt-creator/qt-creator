void foo() {
    QString *str = new QString(QLatin1String("narf"));
    if (!str->isEmpty())
        str->clear();
}
