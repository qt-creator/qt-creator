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

#pragma once

#include "clearcaseplugin.h"

namespace ClearCase {
namespace Internal {

class ClearCaseSync : public QObject
{
    Q_OBJECT
public:
    explicit ClearCaseSync(ClearCasePlugin *plugin, QSharedPointer<StatusMap> statusMap);
    void run(QFutureInterface<void> &future, QStringList &files);

    QStringList updateStatusHotFiles(const QString &viewRoot, int &total);
    void invalidateStatus(const QDir &viewRootDir, const QStringList &files);
    void invalidateStatusAllFiles();
    void processCleartoolLsLine(const QDir &viewRootDir, const QString &buffer);
    void updateTotalFilesCount(const QString view, ClearCaseSettings settings, const int processed);
    void updateStatusForNotManagedFiles(const QStringList &files);

    void syncDynamicView(QFutureInterface<void> &future,
                         const ClearCaseSettings &settings);
    void syncSnapshotView(QFutureInterface<void> &future, QStringList &files,
                          const ClearCaseSettings &settings);

    void processCleartoolLscheckoutLine(const QString &buffer);
signals:
    void updateStreamAndView();

private:
    ClearCasePlugin *const m_plugin;
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

} // namespace Internal
} // namespace ClearCase
