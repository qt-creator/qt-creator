void foo() {
    QString str(QLatin1String("schnurz"));
    if (!str.isEmpty())
        str.clear();
}
