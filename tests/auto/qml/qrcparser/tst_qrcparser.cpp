/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QtTest>
#include <QDebug>
#include <QLocale>

#include <qmljs/qmljsqrcparser.h>

using namespace QmlJS;

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
    QLocale m_locale;
    QrcCache m_cache;
};

void tst_QrcParser::readInData()
{
    QTest::addColumn<QString>("path");

    QDirIterator it(TESTSRCDIR, QStringList("*.qrc"), QDir::Files);
    while (it.hasNext()) {
        const QString fileName = it.next();
        QTest::newRow(fileName.toLatin1()) << it.filePath();
    }
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
        QMap<QString,QStringList> content;
        p->collectFilesInPath(pAtt, &content, true);
        foreach (const QString &fileName, content.keys())
            res.append(pAtt+fileName);
    }
    return res;
}

void tst_QrcParser::firstAtTest()
{
    QFETCH(QString, path);
    QrcParser::Ptr p = QrcParser::parseQrcFile(path);
    foreach (const QString &qrcPath, allPaths(p)) {
        QString s1 = p->firstFileAtPath(qrcPath, m_locale);
        if (s1.isEmpty())
            continue;
        QStringList l;
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
    QrcParser::Ptr p = QrcParser::parseQrcFile(path);
    foreach (const QString &qrcPath, allPaths(p)) {
        if (!qrcPath.endsWith(QLatin1Char('/')))
            continue;
        for (int addDirs = 0; addDirs < 2; ++addDirs) {
            QMap<QString,QStringList> s1;
            p->collectFilesInPath(qrcPath, &s1, addDirs, &m_locale);
            foreach (const QString &k, s1.keys()) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    QCOMPARE(s1.value(k).value(0), p->firstFileAtPath(qrcPath + k, m_locale));
                }
            }
            QMap<QString,QStringList> s2;
            p->collectFilesInPath(qrcPath, &s2, addDirs);
            foreach (const QString &k, s1.keys()) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    QVERIFY(s2.value(k).contains(s1.value(k).at(0)));
                } else {
                    QVERIFY(s2.contains(k));
                }
            }
            foreach (const QString &k, s2.keys()) {
                if (!k.endsWith(QLatin1Char('/'))) {
                    QStringList l;
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
    QVERIFY(m_cache.parsedPath(path).isNull());
    QrcParser::ConstPtr p0 = m_cache.addPath(path);
    QVERIFY(!p0.isNull());
    QrcParser::ConstPtr p1 = m_cache.parsedPath(path);
    QVERIFY(p1.data() == p0.data());
    QrcParser::ConstPtr p2 = m_cache.addPath(path);
    QVERIFY(p2.data() == p1.data());
    QrcParser::ConstPtr p3 = m_cache.parsedPath(path);
    QVERIFY(p3.data() == p2.data());
    QrcParser::ConstPtr p4 = m_cache.updatePath(path);
    QVERIFY(p4.data() != p3.data());
    QrcParser::ConstPtr p5 = m_cache.parsedPath(path);
    QVERIFY(p5.data() == p4.data());
    m_cache.removePath(path);
    QrcParser::ConstPtr p6 = m_cache.parsedPath(path);
    QVERIFY(p6.data() == p5.data());
    m_cache.removePath(path);
    QrcParser::ConstPtr p7 = m_cache.parsedPath(path);
    QVERIFY(p7.isNull());
}

void tst_QrcParser::simpleTest()
{
    QrcParser::Ptr p = QrcParser::parseQrcFile(QString::fromLatin1(TESTSRCDIR).append(QLatin1String("/simple.qrc")));
    QStringList paths = allPaths(p);
    paths.sort();
    QVERIFY(paths == QStringList()
            << QLatin1String("/")
            << QLatin1String("/cut.jpg")
            << QLatin1String("/images/")
            << QLatin1String("/images/copy.png")
            << QLatin1String("/images/cut.png")
            << QLatin1String("/images/new.png")
            << QLatin1String("/images/open.png")
            << QLatin1String("/images/paste.png")
            << QLatin1String("/images/save.png")
            << QLatin1String("/myresources/")
            << QLatin1String("/myresources/cut-img.png"));
    QString frPath = p->firstFileAtPath(QLatin1String("/cut.jpg"), QLocale(QLatin1String("fr_FR")));
    QString refFrPath = QString::fromLatin1(TESTSRCDIR).append(QLatin1String("/cut_fr.jpg"));
    QCOMPARE(frPath, refFrPath);
    QString dePath = p->firstFileAtPath(QLatin1String("/cut.jpg"), QLocale(QLatin1String("de_DE")));
    QString refDePath = QString::fromLatin1(TESTSRCDIR).append(QLatin1String("/cut.jpg"));
    QCOMPARE(dePath, refDePath);
}

void tst_QrcParser::cleanupTestCase()
{
    m_cache.clear();
}

QTEST_APPLESS_MAIN(tst_QrcParser)

#include "tst_qrcparser.moc"
