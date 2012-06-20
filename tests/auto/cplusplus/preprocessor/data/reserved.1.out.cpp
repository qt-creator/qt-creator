# 1 "data/reserved.1.cpp"




int f() {
    foreach (QString &s, QStringList()) {
        doSomething();
    }
    return 1;
}
