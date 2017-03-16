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

#include "clearcasesync.h"
#include "clearcaseconstants.h"

#include <QDir>
#include <QFutureInterface>
#include <QProcess>
#include <QRegExp>
#include <QStringList>
#include <utils/qtcassert.h>

#ifdef WITH_TESTS
#include <QTest>
#include <utils/fileutils.h>
#endif

namespace ClearCase {
namespace Internal {

ClearCaseSync::ClearCaseSync(ClearCasePlugin *plugin, QSharedPointer<StatusMap> statusMap) :
    m_plugin(plugin),
    m_statusMap(statusMap)
{ }

QStringList ClearCaseSync::updateStatusHotFiles(const QString &viewRoot, int &total)
{
    QStringList hotFiles;
    // find all files whose permissions changed OR hijacked files
    // (might have become checked out)
    const StatusMap::Iterator send = m_statusMap->end();
    for (StatusMap::Iterator it = m_statusMap->begin(); it != send; ++it) {
        const QFileInfo fi(viewRoot, it.key());
        const bool permChanged = it.value().permissions != fi.permissions();
        if (permChanged || it.value().status == FileStatus::Hijacked) {
            hotFiles.append(it.key());
            it.value().status = FileStatus::Unknown;
            ++total;
        }
    }
    return hotFiles;
}

// Set status for all files to unknown until we're done indexing
void ClearCaseSync::invalidateStatus(const QDir &viewRootDir,
                                 const QStringList &files)
{
    foreach (const QString &file, files) {
            m_plugin->setStatus(viewRootDir.absoluteFilePath(file), FileStatus::Unknown, false);
    }
}

void ClearCaseSync::invalidateStatusAllFiles()
{
    const StatusMap::ConstIterator send = m_statusMap->constEnd();
    for (StatusMap::ConstIterator it = m_statusMap->constBegin(); it != send; ++it)
        m_plugin->setStatus(it.key(), FileStatus::Unknown, false);
}

void ClearCaseSync::processCleartoolLsLine(const QDir &viewRootDir, const QString &buffer)
{
    const int atatpos = buffer.indexOf(QLatin1String("@@"));

    if (atatpos == -1)
        return;

    // find first whitespace. anything before that is not interesting
    const int wspos = buffer.indexOf(QRegExp(QLatin1String("\\s")));
    const QString absFile =
            viewRootDir.absoluteFilePath(
                QDir::fromNativeSeparators(buffer.left(atatpos)));
    QTC_CHECK(QFile(absFile).exists());
    QTC_CHECK(!absFile.isEmpty());

    QString ccState;
    const QRegExp reState(QLatin1String("^\\s*\\[[^\\]]*\\]")); // [hijacked]; [loaded but missing]
    if (reState.indexIn(buffer, wspos + 1, QRegExp::CaretAtOffset) != -1) {
        ccState = reState.cap();
        if (ccState.indexOf(QLatin1String("hijacked")) != -1)
            m_plugin->setStatus(absFile, FileStatus::Hijacked, true);
        else if (ccState.indexOf(QLatin1String("loaded but missing")) != -1)
            m_plugin->setStatus(absFile, FileStatus::Missing, false);
    }
    else if (buffer.lastIndexOf(QLatin1String("CHECKEDOUT"), wspos) != -1)
        m_plugin->setStatus(absFile, FileStatus::CheckedOut, true);
    // don't care about checked-in files not listed in project
    else if (m_statusMap->contains(absFile))
        m_plugin->setStatus(absFile, FileStatus::CheckedIn, true);
}

void ClearCaseSync::updateTotalFilesCount(const QString view, ClearCaseSettings settings,
                                          const int processed)
{
    settings = m_plugin->settings(); // Might have changed while task was running
    settings.totalFiles[view] = processed;
    m_plugin->setSettings(settings);
}

void ClearCaseSync::updateStatusForNotManagedFiles(const QStringList &files)
{
    foreach (const QString &file, files) {
       QString absFile = QFileInfo(file).absoluteFilePath();
       if (!m_statusMap->contains(absFile))
           m_plugin->setStatus(absFile, FileStatus::NotManaged, false);
    }
}

void ClearCaseSync::syncSnapshotView(QFutureInterface<void> &future, QStringList &files,
                                     const ClearCaseSettings &settings)
{
    QString view = m_plugin->currentView();

    int totalFileCount = files.size();
    const bool hot = (totalFileCount < 10);
    int processed = 0;
    if (!hot)
        totalFileCount = settings.totalFiles.value(view, totalFileCount);

    const QString viewRoot = m_plugin->viewRoot();
    const QDir viewRootDir(viewRoot);

    QStringList args(QLatin1String("ls"));
    if (hot) {
        files << updateStatusHotFiles(viewRoot, totalFileCount);
        args << files;
    } else {
        invalidateStatus(viewRootDir, files);
        args << QLatin1String("-recurse");

        QStringList vobs;
        if (!settings.indexOnlyVOBs.isEmpty())
            vobs = settings.indexOnlyVOBs.split(QLatin1Char(','));
        else
            vobs = m_plugin->ccGetActiveVobs();

        args << vobs;
    }

    // adding 1 for initial sync in which total is not accurate, to prevent finishing
    // (we don't want it to become green)
    future.setProgressRange(0, totalFileCount + 1);
    QProcess process;
    process.setWorkingDirectory(viewRoot);

    const QString program = settings.ccBinaryPath;

    process.start(program, args);
    if (!process.waitForStarted())
        return;
    QString buffer;
    while (process.waitForReadyRead() && !future.isCanceled()) {
        while (process.state() == QProcess::Running &&
               process.bytesAvailable() && !future.isCanceled())
        {
            const QString line = QString::fromLocal8Bit(process.readLine().constData());
            buffer += line;
            if (buffer.endsWith(QLatin1Char('\n')) || process.atEnd()) {
                processCleartoolLsLine(viewRootDir, buffer);
                buffer.clear();
                future.setProgressValue(qMin(totalFileCount, ++processed));
            }
        }
    }

    if (!future.isCanceled()) {
        updateStatusForNotManagedFiles(files);
        future.setProgressValue(totalFileCount + 1);
        if (!hot)
            updateTotalFilesCount(view, settings, processed);
    }

    if (process.state() == QProcess::Running)
        process.kill();

    process.waitForFinished();
}

void ClearCaseSync::processCleartoolLscheckoutLine(const QString &buffer)
{
    QString absFile = buffer.trimmed();
    m_plugin->setStatus(absFile, FileStatus::CheckedOut, true);
}

///
/// Update the file status for dynamic views.
///
void ClearCaseSync::syncDynamicView(QFutureInterface<void> &future,
                                    const ClearCaseSettings& settings)
{
    // Always invalidate status for all files
    invalidateStatusAllFiles();

    QStringList args(QLatin1String("lscheckout"));
    args << QLatin1String("-avobs")
         << QLatin1String("-me")
         << QLatin1String("-cview")
         << QLatin1String("-s");

    const QString viewRoot = m_plugin->viewRoot();

    QProcess process;
    process.setWorkingDirectory(viewRoot);

    const QString program = settings.ccBinaryPath;
    process.start(program, args);
    if (!process.waitForStarted())
        return;

    QString buffer;
    int processed = 0;
    while (process.waitForReadyRead() && !future.isCanceled()) {
        while (process.state() == QProcess::Running &&
               process.bytesAvailable() && !future.isCanceled()) {
            const QString line = QString::fromLocal8Bit(process.readLine().constData());
            buffer += line;
            if (buffer.endsWith(QLatin1Char('\n')) || process.atEnd()) {
                processCleartoolLscheckoutLine(buffer);
                buffer.clear();
                future.setProgressValue(++processed);
            }
        }
    }

    if (process.state() == QProcess::Running)
        process.kill();

    process.waitForFinished();
}

void ClearCaseSync::run(QFutureInterface<void> &future, QStringList &files)
{
    ClearCaseSettings settings = m_plugin->settings();
    if (settings.disableIndexer)
        return;

    const QString program = settings.ccBinaryPath;
    if (program.isEmpty())
        return;

    // refresh activities list
    if (m_plugin->isUcm())
        m_plugin->refreshActivities();

    QString view = m_plugin->currentView();
    if (view.isEmpty())
        emit updateStreamAndView();

    if (m_plugin->isDynamic())
        syncDynamicView(future, settings);
    else
        syncSnapshotView(future, files, settings);
}

#ifdef WITH_TESTS
namespace {
class TempFile
{
public:
    TempFile(const QString &fileName)
        : m_fileName(fileName)
    {
        Utils::FileSaver srcSaver(fileName);
        srcSaver.write(QByteArray());
        srcSaver.finalize();

    }

    QString fileName() const { return m_fileName; }

    ~TempFile()
    {
        QVERIFY(QFile::remove(m_fileName));
    }

private:
    const QString m_fileName;
};
}

void ClearCaseSync::verifyParseStatus(const QString &fileName,
                                 const QString &cleartoolLsLine,
                                 const FileStatus::Status status)
{
    QCOMPARE(m_statusMap->count(), 0);
    processCleartoolLsLine(QDir(QLatin1String("/")), cleartoolLsLine);

    if (status == FileStatus::CheckedIn) {
        // The algorithm doesn't store checked in files in the index, unless it was there already
        QCOMPARE(m_statusMap->count(), 0);
        QCOMPARE(m_statusMap->contains(fileName), false);
        m_plugin->setStatus(fileName, FileStatus::Unknown, false);
        processCleartoolLsLine(QDir(QLatin1String("/")), cleartoolLsLine);
    }

    QCOMPARE(m_statusMap->count(), 1);
    QCOMPARE(m_statusMap->contains(fileName), true);
    QCOMPARE(m_statusMap->value(fileName).status, status);

    QCOMPARE(m_statusMap->contains(QLatin1String(("notexisting"))), false);
}

void ClearCaseSync::verifyFileNotManaged()
{
    QCOMPARE(m_statusMap->count(), 0);
    TempFile temp(QDir::currentPath() + QLatin1String("/notmanaged.cpp"));
    const QString fileName = temp.fileName();

    updateStatusForNotManagedFiles(QStringList(fileName));

    QCOMPARE(m_statusMap->count(), 1);

    QCOMPARE(m_statusMap->contains(fileName), true);
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::NotManaged);
}

void ClearCaseSync::verifyFileCheckedOutDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);

    QString fileName(QLatin1String("/hello.C"));
    processCleartoolLscheckoutLine(fileName);

    QCOMPARE(m_statusMap->count(), 1);

    QVERIFY(m_statusMap->contains(fileName));
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::CheckedOut);

    QVERIFY(!m_statusMap->contains(QLatin1String(("notexisting"))));
}

void ClearCaseSync::verifyFileCheckedInDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);

    QString fileName(QLatin1String("/hello.C"));

    // checked in files are not kept in the index
    QCOMPARE(m_statusMap->count(), 0);
    QCOMPARE(m_statusMap->contains(fileName), false);
}

void ClearCaseSync::verifyFileNotManagedDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);
    TempFile temp(QDir::currentPath() + QLatin1String("/notmanaged.cpp"));
    const QString fileName = temp.fileName();

    updateStatusForNotManagedFiles(QStringList(fileName));

    QCOMPARE(m_statusMap->count(), 1);

    QVERIFY(m_statusMap->contains(fileName));
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::NotManaged);
}

#endif


} // namespace Internal
} // namespace ClearCase
