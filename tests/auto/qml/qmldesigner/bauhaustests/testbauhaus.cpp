/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    m_executable = m_creatorDir + "/bin/qtcreator";
#endif

    Q_ASSERT(QFileInfo(m_executable).exists());
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
    QTest::qWait(10000);
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

void TestBauhaus::loadCreator_data()
{
    QTest::addColumn<QString>("filePath");
    foreach (const QString &file, findAllQmlFiles(QDir(m_creatorDir))) {
        QTest::newRow("file") << file;
    }
}

void TestBauhaus::loadCreator()
{
    QFETCH(QString, filePath);
    if (!loadFile(filePath))
        QFAIL(filePath.toAscii());
}

QTEST_MAIN(TestBauhaus);
