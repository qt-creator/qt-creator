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


TestBauhaus::TestBauhaus()
    : QObject()
{
    foreach (const QString &string, QProcess::systemEnvironment()) {
        if (string.contains("qtdir", Qt::CaseInsensitive)) {
            m_qtDir = string.split("=").last();
            break;
        }
    }
    Q_ASSERT(!m_qtDir.isEmpty());

    m_creatorDir = QString(WORKDIR) + "../../../../..";
#ifdef Q_OS_WIN
    m_executable = m_creatorDir + "/bin/qtcreator.exe";
#else
    m_executable = m_creatorDir + "/bin/qtcreator.bin";
#endif

    Q_ASSERT(QFileInfo(m_executable).exists());
}

void TestBauhaus::initTestCase()
{
}

bool TestBauhaus::loadFile(const QString &fileName)
{
    QProcess process;
    qDebug() << "starting: " << fileName;
    Q_ASSERT(QFileInfo(fileName).exists());

    process.start(m_executable, QStringList() << fileName);
    if (!process.waitForStarted())
        return false;
    if (!QProcess::Running == process.state()) {
        return false;
    }
    QTest::qWait(5000);
    if (!QProcess::Running == process.state()) {
        return false;
    }
    return true;
}

QStringList findAllQmlFiles(const QDir &dir)
{
    QStringList files;
    foreach (const QString &file, dir.entryList(QStringList() << "*.qml", QDir::Files)) {
        files += dir.absoluteFilePath(file);
    }

    foreach (const QString &directory, dir.entryList(QStringList(), QDir::AllDirs | QDir::NoDotAndDotDot))
        files += findAllQmlFiles(QDir(dir.absoluteFilePath(directory)));
    return files;
}

void TestBauhaus::cleanupTestCase()
{
}

void TestBauhaus::loadExamples_data()
{
    QTest::addColumn<QString>("filePath");
    foreach (const QString &file, findAllQmlFiles(QDir(m_qtDir + "/examples/declarative"))) {
        QTest::newRow("file") << file;
    }
}

void TestBauhaus::loadExamples()
{
    QFETCH(QString, filePath);
    if (!loadFile(filePath))
        QFAIL(filePath.toAscii());
}

void TestBauhaus::loadDemos_data()
{
    QTest::addColumn<QString>("filePath");
    foreach (const QString &file, findAllQmlFiles(QDir(m_qtDir + "/demos/declarative"))) {
        QTest::newRow("file") << file;
    }
}

void TestBauhaus::loadDemos()
{
    QFETCH(QString, filePath);
    if (!loadFile(filePath))
        QFAIL(filePath.toAscii());
}


QTEST_MAIN(TestBauhaus);
