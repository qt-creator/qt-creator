// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/qrcparser.h>

#include <QDebug>
#include <QLocale>
#include <QTest>

using namespace Utils;

class tst_QrcParser: public QObject
{
    Q_OBJECT

public:
    void readInData();
    QStringList allPaths(QrcParser::ConstPtr p);

private slots:
    void firstAtTest_data() { readInData(); }
    void firstInTest_data() { readInData(); }
    void cacheTest_data() { readInData(); }
    void firstAtTest();
    void firstInTest();
    void cacheTest();
    void simpleTest();
    void cleanupTestCase();

private:
    const FilePath m_testSrcDir{TESTSRCDIR};
    QLocale m_locale;
    QrcCache m_cache;
};

void tst_QrcParser::readInData()
{
    QTest::addColumn<QString>("path");

    for (const FilePath &qrcFile : m_testSrcDir.dirEntries({{"*.qrc"}, QDir::Files}))
        QTest::newRow(qrcFile.fileName().toLatin1()) << qrcFile.path();
}

QStringList tst_QrcParser::allPaths(QrcParser::ConstPtr p)
{
    QStringList res;
    res << QLatin1String("/");
    int iPos = 0;
    while (iPos < res.size()) {
        QString pAtt = res.at(iPos++);
        if (!pAtt.endsWith(QLatin1Char('/')))
            continue;
        QMap<QString, FilePaths> content;
        p->collectFilesInPath(pAtt, &content, true);
        const QStringList fileNames = content.keys();
        for (const QString &fileName : fileNames)
            res.append(pAtt+fileName);
    }
    return res;
}

void tst_QrcParser::firstAtTest()
{
    QFETCH(QString, path);
    QrcParser::Ptr p = QrcParser::parseQrcFile(FilePath::fromString(path), QString());
    const QStringList paths = allPaths(p);
    for (const QString &qrcPath : paths) {
        FilePath s1 = p->firstFileAtPath(qrcPath, m_locale);
        if (s1.isEmpty())
            continue;
        FilePaths l;
        p->collectFilesAtPath(qrcPath, &l, &m_locale);
        QCOMPARE(l.value(0), s1);
        l.clear();
        p->collectFilesAtPath(qrcPath, &l);
        QVERIFY(l.contains(s1));
    }
}

void tst_QrcParser::firstInTest()
{
    QFETCH(QString, path);
    QrcParser::Ptr p = QrcParser::parseQrcFile(FilePath::fromString(path), QString());
    const QStringList paths = allPaths(p);
    for (const QString &qrcPath : paths) {
        if (!qrcPath.endsWith(QLatin1Char('/')))
            continue;
        for (int addDirs = 0; addDirs < 2; ++addDirs) {
            QMap<QString, FilePaths> s1;
            p->collectFilesInPath(qrcPath, &s1, addDirs, &m_locale);
            const QStringList keys = s1.keys();
            for (const QString &k : keys) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    QCOMPARE(s1.value(k).value(0), p->firstFileAtPath(qrcPath + k, m_locale));
                }
            }
            QMap<QString, FilePaths> s2;
            p->collectFilesInPath(qrcPath, &s2, addDirs);
            for (const QString &k : keys) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    QVERIFY(s2.value(k).contains(s1.value(k).at(0)));
                } else {
                    QVERIFY(s2.contains(k));
                }
            }
            const QStringList keys2 = s2.keys();
            for (const QString &k : keys2) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    FilePaths l;
                    p->collectFilesAtPath(qrcPath + k, &l);
                    QCOMPARE(s2.value(k), l);
                } else {
                    QVERIFY(s2.value(k).isEmpty());
                }
            }
        }
    }
}

void tst_QrcParser::cacheTest()
{
    QFETCH(QString, path);
    FilePath filePath = FilePath::fromString(path);
    QVERIFY(!m_cache.parsedPath(filePath));
    QrcParser::ConstPtr p0 = m_cache.addPath(filePath, QString());
    QVERIFY(p0);
    QrcParser::ConstPtr p1 = m_cache.parsedPath(filePath);
    QVERIFY(p1.get() == p0.get());
    QrcParser::ConstPtr p2 = m_cache.addPath(filePath, QString());
    QVERIFY(p2.get() == p1.get());
    QrcParser::ConstPtr p3 = m_cache.parsedPath(filePath);
    QVERIFY(p3.get() == p2.get());
    QrcParser::ConstPtr p4 = m_cache.updatePath(filePath, QString());
    QVERIFY(p4.get() != p3.get());
    QrcParser::ConstPtr p5 = m_cache.parsedPath(filePath);
    QVERIFY(p5.get() == p4.get());
    m_cache.removePath(filePath);
    QrcParser::ConstPtr p6 = m_cache.parsedPath(filePath);
    QVERIFY(p6.get() == p5.get());
    m_cache.removePath(filePath);
    QrcParser::ConstPtr p7 = m_cache.parsedPath(filePath);
    QVERIFY(!p7);
}

void tst_QrcParser::simpleTest()
{
    QrcParser::Ptr p = QrcParser::parseQrcFile(m_testSrcDir.pathAppended("simple.qrc"), QString());
    QStringList paths = allPaths(p);
    paths.sort();
    QVERIFY(paths == QStringList({ "/", "/cut.jpg", "/images/", "/images/copy.png",
                                   "/images/cut.png", "/images/new.png", "/images/open.png",
                                   "/images/paste.png", "/images/save.png", "/myresources/",
                                   "/myresources/cut-img.png" }));
    FilePath frPath = p->firstFileAtPath(QLatin1String("/cut.jpg"), QLocale(QLatin1String("fr_FR")));
    FilePath refFrPath = m_testSrcDir.pathAppended("cut_fr.jpg");
    QCOMPARE(frPath, refFrPath);
    FilePath dePath = p->firstFileAtPath(QLatin1String("/cut.jpg"), QLocale(QLatin1String("de_DE")));
    FilePath refDePath = m_testSrcDir.pathAppended("cut.jpg");
    QCOMPARE(dePath, refDePath);
}

void tst_QrcParser::cleanupTestCase()
{
    m_cache.clear();
}

QTEST_APPLESS_MAIN(tst_QrcParser)

#include "tst_qrcparser.moc"
