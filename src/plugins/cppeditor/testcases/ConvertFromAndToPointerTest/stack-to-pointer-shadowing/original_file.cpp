void foo() {
    QString @str;
    str.clear();
    {
        QString str;
        str.clear();
    }
    f1(str);
}
