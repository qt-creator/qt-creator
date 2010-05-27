
#include "gdb/gdbmi.h"

#include <QtTest>


class tst_Version : public QObject
{
    Q_OBJECT

public:
    tst_Version() {}

private slots:
    void version();
    void version_data();
};

void tst_Version::version()
{
    QFETCH(QString, msg);
    QFETCH(int, gdbVersion);
    QFETCH(int, gdbBuildVersion);
    QFETCH(bool, isMacGdb);
    int v = 0, bv = 0;
    bool mac = true;
    Debugger::Internal::extractGdbVersion(msg, &v, &bv, &mac);
    qDebug() << msg << " -> " << v << bv << mac;
    QCOMPARE(v, gdbVersion);
    QCOMPARE(bv, gdbBuildVersion);
    QCOMPARE(mac, isMacGdb);
}

void tst_Version::version_data()
{
    QTest::addColumn<QString>("msg");
    QTest::addColumn<int>("gdbVersion");
    QTest::addColumn<int>("gdbBuildVersion");
    QTest::addColumn<bool>("isMacGdb");

    QTest::newRow("Debian")
        << "GNU gdb (GDB) 7.0.1-debian"
        << 70001 << 0 << false;

    QTest::newRow("CVS 7.0.90")
        << "GNU gdb (GDB) 7.0.90.20100226-cvs"
        << 70090 << 20100226 << false;

    QTest::newRow("Ubuntu Lucid")
        << "GNU gdb (GDB) 7.1-ubuntu"
        << 70100 << 0 << false;

    QTest::newRow("Fedora 13")
        << "GNU gdb (GDB) Fedora (7.1-22.fc13)"
        << 70100 << 22 << false;

    QTest::newRow("Gentoo")
        << "GNU gdb (Gentoo 7.1 p1) 7.1"
        << 70100 << 1 << false;

    QTest::newRow("Fedora EL5")
        << "GNU gdb Fedora (6.8-37.el5)"
        << 60800 << 37 << false;

    QTest::newRow("SUSE")
        << "GNU gdb (GDB) SUSE (6.8.91.20090930-2.4)"
        << 60891 << 20090930 << false;

    QTest::newRow("Apple")
        << "GNU gdb 6.3.50-20050815 (Apple version gdb-1461.2)"
        << 60350 << 1461 << true;
}


int main(int argc, char *argv[])
{
    tst_Version test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_version.moc"

