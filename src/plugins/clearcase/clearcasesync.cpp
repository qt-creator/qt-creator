// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clearcasesync.h"

#include "clearcasesettings.h"

#include <QDir>
#include <QRegularExpression>
#include <QStringList>

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QPromise>

#ifdef WITH_TESTS
#include <QTest>
#include <utils/fileutils.h>
#endif

using namespace Utils;

namespace ClearCase::Internal {

static void runProcess(QPromise<void> &promise, const ClearCaseSettings &settings,
                       const QStringList &args,
                       std::function<void(const QString &buffer, int processed)> processLine)
{
    const QString viewRoot = ClearCasePlugin::viewData().root;
    Process process;
    process.setWorkingDirectory(FilePath::fromString(viewRoot));
    process.setCommand({settings.ccBinaryPath, args});
    process.start();
    if (!process.waitForStarted())
        return;

    int processed = 0;
    QString buffer;
    while (process.waitForReadyRead() && !promise.isCanceled()) {
        buffer += QString::fromLocal8Bit(process.readAllRawStandardOutput());
        int index = buffer.indexOf('\n');
        while (index != -1) {
            const QString line = buffer.left(index + 1);
            processLine(line, ++processed);
            buffer = buffer.mid(index + 1);
            index = buffer.indexOf('\n');
        }
    }
    if (!buffer.isEmpty())
        processLine(buffer, ++processed);
}

ClearCaseSync::ClearCaseSync(QSharedPointer<StatusMap> statusMap) :
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
void ClearCaseSync::invalidateStatus(const QDir &viewRootDir, const QStringList &files)
{
    for (const QString &file : files)
        ClearCasePlugin::setStatus(viewRootDir.absoluteFilePath(file), FileStatus::Unknown, false);
}

void ClearCaseSync::invalidateStatusAllFiles()
{
    const StatusMap::ConstIterator send = m_statusMap->constEnd();
    for (StatusMap::ConstIterator it = m_statusMap->constBegin(); it != send; ++it)
        ClearCasePlugin::setStatus(it.key(), FileStatus::Unknown, false);
}

void ClearCaseSync::processCleartoolLsLine(const QDir &viewRootDir, const QString &buffer)
{
    const int atatpos = buffer.indexOf("@@");

    if (atatpos == -1)
        return;

    // find first whitespace. anything before that is not interesting
    const int wspos = buffer.indexOf(QRegularExpression("\\s"));
    const QString absFile =
            viewRootDir.absoluteFilePath(
                QDir::fromNativeSeparators(buffer.left(atatpos)));
    QTC_CHECK(QFileInfo::exists(absFile));
    QTC_CHECK(!absFile.isEmpty());

    const QRegularExpression reState("^\\s*\\[[^\\]]*\\]"); // [hijacked]; [loaded but missing]
    const QRegularExpressionMatch match = reState.match(buffer.mid(wspos + 1));
    if (match.hasMatch()) {
        const QString ccState = match.captured();
        if (ccState.indexOf("hijacked") != -1)
            ClearCasePlugin::setStatus(absFile, FileStatus::Hijacked, true);
        else if (ccState.indexOf("loaded but missing") != -1)
            ClearCasePlugin::setStatus(absFile, FileStatus::Missing, false);
    }
    else if (buffer.lastIndexOf("CHECKEDOUT", wspos) != -1)
        ClearCasePlugin::setStatus(absFile, FileStatus::CheckedOut, true);
    // don't care about checked-in files not listed in project
    else if (m_statusMap->contains(absFile))
        ClearCasePlugin::setStatus(absFile, FileStatus::CheckedIn, true);
}

void ClearCaseSync::updateTotalFilesCount(const Key &view, const int processed)
{
    ClearCaseSettings settings = ClearCasePlugin::settings();
    settings.totalFiles[view] = processed;
    ClearCasePlugin::setSettings(settings);
}

void ClearCaseSync::updateStatusForNotManagedFiles(const QStringList &files)
{
    for (const QString &file : files) {
        const QString absFile = QFileInfo(file).absoluteFilePath();
        if (!m_statusMap->contains(absFile))
            ClearCasePlugin::setStatus(absFile, FileStatus::NotManaged, false);
    }
}

void ClearCaseSync::syncSnapshotView(QPromise<void> &promise, QStringList &files,
                                     const ClearCaseSettings &settings)
{
    const Key view = keyFromString(ClearCasePlugin::viewData().name);

    int totalFileCount = files.size();
    const bool hot = (totalFileCount < 10);
    if (!hot)
        totalFileCount = settings.totalFiles.value(view, totalFileCount);

    const QString viewRoot = ClearCasePlugin::viewData().root;
    const QDir viewRootDir(viewRoot);

    QStringList args("ls");
    if (hot) {
        files << updateStatusHotFiles(viewRoot, totalFileCount);
        args << files;
    } else {
        invalidateStatus(viewRootDir, files);
        args << "-recurse";

        QStringList vobs;
        if (!settings.indexOnlyVOBs.isEmpty())
            vobs = settings.indexOnlyVOBs.split(QLatin1Char(','));
        else
            vobs = ClearCasePlugin::ccGetActiveVobs();

        args << vobs;
    }

    // adding 1 for initial sync in which total is not accurate, to prevent finishing
    // (we don't want it to become green)
    promise.setProgressRange(0, totalFileCount + 1);

    int totalProcessed = 0;
    runProcess(promise, settings, args, [&](const QString &buffer, int processed) {
        processCleartoolLsLine(viewRootDir, buffer);
        promise.setProgressValue(qMin(totalFileCount, processed));
        totalProcessed = processed;
    });

    if (!promise.isCanceled()) {
        updateStatusForNotManagedFiles(files);
        promise.setProgressValue(totalFileCount + 1);
        if (!hot)
            updateTotalFilesCount(view, totalProcessed);
    }
}

void ClearCaseSync::processCleartoolLscheckoutLine(const QString &buffer)
{
    const QString absFile = buffer.trimmed();
    ClearCasePlugin::setStatus(absFile, FileStatus::CheckedOut, true);
}

///
/// Update the file status for dynamic views.
///
void ClearCaseSync::syncDynamicView(QPromise<void> &promise, const ClearCaseSettings& settings)
{
    // Always invalidate status for all files
    invalidateStatusAllFiles();

    const QStringList args({"lscheckout", "-avobs", "-me", "-cview", "-s"});

    runProcess(promise, settings, args, [&](const QString &buffer, int processed) {
        processCleartoolLscheckoutLine(buffer);
        promise.setProgressValue(processed);
    });
}

void ClearCaseSync::run(QPromise<void> &promise, QStringList &files)
{
    ClearCaseSettings settings = ClearCasePlugin::settings();
    if (settings.disableIndexer)
        return;

    if (!settings.ccBinaryPath.isExecutableFile())
        return;

    // refresh activities list
    if (ClearCasePlugin::viewData().isUcm)
        ClearCasePlugin::refreshActivities();

    const QString view = ClearCasePlugin::viewData().name;
    if (view.isEmpty())
        emit updateStreamAndView();

    if (ClearCasePlugin::viewData().isDynamic)
        syncDynamicView(promise, settings);
    else
        syncSnapshotView(promise, files, settings);
}

#ifdef WITH_TESTS
namespace {
class TempFile
{
public:
    TempFile(const QString &fileName)
        : m_fileName(fileName)
    {
        FileSaver srcSaver(FilePath::fromString(fileName));
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
    processCleartoolLsLine(QDir("/"), cleartoolLsLine);

    if (status == FileStatus::CheckedIn) {
        // The algorithm doesn't store checked in files in the index, unless it was there already
        QCOMPARE(m_statusMap->count(), 0);
        QCOMPARE(m_statusMap->contains(fileName), false);
        ClearCasePlugin::setStatus(fileName, FileStatus::Unknown, false);
        processCleartoolLsLine(QDir("/"), cleartoolLsLine);
    }

    QCOMPARE(m_statusMap->count(), 1);
    QCOMPARE(m_statusMap->contains(fileName), true);
    QCOMPARE(m_statusMap->value(fileName).status, status);

    QCOMPARE(m_statusMap->contains("notexisting"), false);
}

void ClearCaseSync::verifyFileNotManaged()
{
    QCOMPARE(m_statusMap->count(), 0);
    TempFile temp(QDir::currentPath() + "/notmanaged.cpp");
    const QString fileName = temp.fileName();

    updateStatusForNotManagedFiles(QStringList(fileName));

    QCOMPARE(m_statusMap->count(), 1);

    QCOMPARE(m_statusMap->contains(fileName), true);
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::NotManaged);
}

void ClearCaseSync::verifyFileCheckedOutDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);

    const QString fileName("/hello.C");
    processCleartoolLscheckoutLine(fileName);

    QCOMPARE(m_statusMap->count(), 1);

    QVERIFY(m_statusMap->contains(fileName));
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::CheckedOut);

    QVERIFY(!m_statusMap->contains("notexisting"));
}

void ClearCaseSync::verifyFileCheckedInDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);

    const QString fileName("/hello.C");

    // checked in files are not kept in the index
    QCOMPARE(m_statusMap->count(), 0);
    QCOMPARE(m_statusMap->contains(fileName), false);
}

void ClearCaseSync::verifyFileNotManagedDynamicView()
{
    QCOMPARE(m_statusMap->count(), 0);
    TempFile temp(QDir::currentPath() + "/notmanaged.cpp");
    const QString fileName = temp.fileName();

    updateStatusForNotManagedFiles(QStringList(fileName));

    QCOMPARE(m_statusMap->count(), 1);

    QVERIFY(m_statusMap->contains(fileName));
    QCOMPARE(m_statusMap->value(fileName).status, FileStatus::NotManaged);
}

#endif

} // ClearCase::Internal
