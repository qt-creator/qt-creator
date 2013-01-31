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

#include "clearcasesync.h"
#include "clearcaseconstants.h"

#include <QDir>
#include <QFutureInterface>
#include <QProcess>
#include <QStringList>

namespace ClearCase {
namespace Internal {

ClearCaseSync::ClearCaseSync(ClearCasePlugin *plugin, QSharedPointer<StatusMap> statusMap) :
    m_plugin(plugin),
    m_statusMap(statusMap)
{
}

void ClearCaseSync::run(QFutureInterface<void> &future, const QString &topLevel, QStringList &files)
{
    ClearCaseSettings settings = m_plugin->settings();
    QString program = settings.ccBinaryPath;
    if (program.isEmpty())
        return;
    int total = files.size();
    bool hot = (total < 10);
    int processed = 0;
    QString view = m_plugin->currentView();
    if (view.isEmpty())
        emit updateStreamAndView();
    if (!hot)
        total = settings.totalFiles.value(view, total);

    // refresh activities list
    if (m_plugin->isUcm())
        m_plugin->refreshActivities();

    if (settings.disableIndexer)
        return;
    QStringList vobs;
    if (!settings.indexOnlyVOBs.isEmpty())
        vobs = settings.indexOnlyVOBs.split(QLatin1Char(','));
    else
        vobs = m_plugin->ccGetActiveVobs();
    QDir topLevelDir(topLevel);
    QStringList args(QLatin1String("ls"));
    if (hot) {
        // find all files whose permissions changed OR hijacked files
        // (might have become checked out)
        foreach (const QString &file, m_statusMap->keys()) {
            bool permChanged =
                    m_statusMap->value(file).permissions != QFileInfo(topLevel, file).permissions();
            if (permChanged || m_statusMap->value(file).status == FileStatus::Hijacked) {
                files.append(file);
                (*m_statusMap)[file].status = FileStatus::Unknown;
                ++total;
            }
        }
        args << files;
    } else {
        foreach (const QString &file, files)
            m_plugin->setStatus(topLevelDir.relativeFilePath(file), FileStatus::Unknown, false);
        args << QLatin1String("-recurse");
        args << vobs;
    }

    // adding 1 for initial sync in which total is not accurate, to prevent finishing
    // (we don't want it to become green)
    future.setProgressRange(0, total + 1);
    QProcess process;
    process.setWorkingDirectory(topLevel);

    process.start(program, args);
    if (!process.waitForStarted())
        return;
    QString buffer;
    while (process.waitForReadyRead() && !future.isCanceled()) {
        while (process.state() == QProcess::Running &&
               process.bytesAvailable() && !future.isCanceled())
        {
            QString line = QString::fromLocal8Bit(process.readLine().constData());

            buffer += line;
            if (buffer.endsWith(QLatin1Char('\n')) || process.atEnd()) {
                int atatpos = buffer.indexOf(QLatin1String("@@"));
                if (atatpos != -1) { // probably managed file
                    // find first whitespace. anything before that is not interesting
                    int wspos = buffer.indexOf(QRegExp(QLatin1String("\\s")));
                    const QString file = QDir::fromNativeSeparators(buffer.left(atatpos));
                    QString ccState;
                    QRegExp reState(QLatin1String("^\\s*\\[[^\\]]*\\]")); // [hijacked]; [loaded but missing]
                    if (reState.indexIn(buffer, wspos + 1, QRegExp::CaretAtOffset) != -1) {
                        ccState = reState.cap();
                        if (ccState.indexOf(QLatin1String("hijacked")) != -1)
                            m_plugin->setStatus(file, FileStatus::Hijacked, true);
                        else if (ccState.indexOf(QLatin1String("loaded but missing")) != -1)
                            m_plugin->setStatus(file, FileStatus::Missing, false);
                    }
                    else if (buffer.lastIndexOf(QLatin1String("CHECKEDOUT"), wspos) != -1)
                        m_plugin->setStatus(file, FileStatus::CheckedOut, true);
                    // don't care about checked-in files not listed in project
                    else if (m_statusMap->contains(file))
                        m_plugin->setStatus(file, FileStatus::CheckedIn, true);
                }
                buffer.clear();
                future.setProgressValue(qMin(total, ++processed));
            }
        }
    }

    if (!future.isCanceled()) {
        foreach (const QString &file, files) {
            QString relFile = topLevelDir.relativeFilePath(file);
            if (m_statusMap->value(relFile).status == FileStatus::Unknown)
                m_plugin->setStatus(relFile, FileStatus::NotManaged, false);
        }
        future.setProgressValue(total + 1);
        if (!hot) {
            settings = m_plugin->settings(); // Might have changed while task was running
            settings.totalFiles[view] = processed;
            m_plugin->setSettings(settings);
        }
    }
    if (process.state() == QProcess::Running)
        process.kill();
    process.waitForFinished();
}

} // namespace Internal
} // namespace ClearCase
