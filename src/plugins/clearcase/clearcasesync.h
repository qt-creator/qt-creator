// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clearcaseplugin.h"

QT_BEGIN_NAMESPACE
class QDir;
template <typename T>
class QFutureInterface;
QT_END_NAMESPACE

namespace ClearCase::Internal {

class ClearCaseSync : public QObject
{
    Q_OBJECT
public:
    explicit ClearCaseSync(QSharedPointer<StatusMap> statusMap);
    void run(QFutureInterface<void> &future, QStringList &files);

    QStringList updateStatusHotFiles(const QString &viewRoot, int &total);
    void invalidateStatus(const QDir &viewRootDir, const QStringList &files);
    void invalidateStatusAllFiles();
    void processCleartoolLsLine(const QDir &viewRootDir, const QString &buffer);
    void updateTotalFilesCount(const QString &view, ClearCaseSettings settings,
                               const int processed);
    void updateStatusForNotManagedFiles(const QStringList &files);

    void syncDynamicView(QFutureInterface<void> &future,
                         const ClearCaseSettings &settings);
    void syncSnapshotView(QFutureInterface<void> &future, QStringList &files,
                          const ClearCaseSettings &settings);

    void processCleartoolLscheckoutLine(const QString &buffer);
signals:
    void updateStreamAndView();

private:
    QSharedPointer<StatusMap> m_statusMap;

#ifdef WITH_TESTS
public slots:
    void verifyParseStatus(const QString &fileName, const QString &cleartoolLsLine,
                           const FileStatus::Status);
    void verifyFileNotManaged();

    void verifyFileCheckedOutDynamicView();
    void verifyFileCheckedInDynamicView();
    void verifyFileNotManagedDynamicView();
#endif
};

} // ClearCase::Internal
