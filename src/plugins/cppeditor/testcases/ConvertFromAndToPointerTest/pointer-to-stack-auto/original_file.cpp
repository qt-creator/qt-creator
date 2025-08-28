void foo() {
    auto @str = new QString(QLatin1String(\foo\));
    if (!str->isEmpty())
        str->clear();
    f1(*str);
    f2(str);
}
