void foo() {
    QString @str(QLatin1String("narf"));
    if (!str.isEmpty())
        str.clear();
}
