/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "testbauhaus.h"

#include <QProcess>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

QStringList failures;


TestBauhaus::TestBauhaus()
    : QObject()
{
}

void TestBauhaus::initTestCase()
{
}

void loadFile(const QString &fileName)
{
    QProcess process;
    qDebug() << "starting: " << fileName;
    QVERIFY(QFileInfo(fileName).exists());
#ifdef Q_OS_WIN
    const QString bauhausExecutable = "bauhaus.exe";
#else
    const QString bauhausExecutable = QDir::current().absoluteFilePath("bauhaus");
#endif
    QVERIFY(QFileInfo(bauhausExecutable).isExecutable());
    process.start(bauhausExecutable, QStringList() << fileName);
    if (!process.waitForStarted())
        QFAIL(fileName.toLatin1());
    if (!QProcess::Running == process.state()) {
        QFAIL(fileName.toLatin1());
        failures << fileName;
    }
    QTest::qWait(5000);
    if (!QProcess::Running == process.state()) {
        QFAIL(fileName.toLatin1());
        failures << fileName;
    }
}

void loadAllFiles(const QString &path)
{
    QDir::setCurrent(WORKDIR);
    QVERIFY(QFileInfo(path).exists());
    QDir dir(path);
    foreach (const QString &file, dir.entryList(QStringList() << "*.qml", QDir::Files))
        loadFile(path + "/" + file);
    foreach (const QString &directory, dir.entryList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot))
        loadAllFiles(path + "/" + directory);
}

void TestBauhaus::cleanupTestCase()
{
}

void TestBauhaus::loadExamples()
{
    failures.clear();
    QString qtdir;
    foreach (const QString &string, QProcess::systemEnvironment())
        if (string.contains("qtdir", Qt::CaseInsensitive))
            qtdir = string.split("=").last();
    if (qtdir.isEmpty())
        qWarning() << "QTDIR has to be set";
    QVERIFY(!qtdir.isEmpty());
    QVERIFY(QFileInfo(qtdir + "/examples/declarative").exists());
    loadAllFiles(qtdir + "/examples/declarative");
    qDebug() << failures;
    QVERIFY(failures.isEmpty());
}

void TestBauhaus::loadDemos()
{
    failures.clear();
    QString qtdir;
    foreach (const QString &string, QProcess::systemEnvironment())
        if (string.contains("qtdir", Qt::CaseInsensitive))
            qtdir = string.split("=").last();
    if (qtdir.isEmpty())
        qWarning() << "QTDIR has to be set";
    QVERIFY(!qtdir.isEmpty());
    QVERIFY(QFileInfo(qtdir + "/demos/declarative").exists());
    loadAllFiles(qtdir + "/demos/declarative");
    qDebug() << failures;
    QVERIFY(failures.isEmpty());
}


QTEST_MAIN(TestBauhaus);
