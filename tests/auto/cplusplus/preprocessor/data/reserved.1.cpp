#define Q_FOREACH(variable, container) foobar(variable, container)
#define foreach Q_FOREACH


int f() {
    foreach (QString &s, QStringList()) {
        doSomething();
    }
    return 1;
}
