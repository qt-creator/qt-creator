/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "shellcommand.h"

#include "icore.h"
#include "progressmanager/progressmanager.h"

#include <QFutureInterface>
#include <QFutureWatcher>

namespace Core {

ShellCommand::ShellCommand(const QString &workingDirectory, const QProcessEnvironment &environment) :
    Utils::ShellCommand(workingDirectory, environment)
{
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
            this, &ShellCommand::coreAboutToClose);
}

void ShellCommand::addTask(QFuture<void> &future)
{
    const QString name = displayName();
    const auto id = Core::Id::fromString(name + QLatin1String(".action"));
    if (hasProgressParser()) {
        ProgressManager::addTask(future, name, id);
    } else {
        // add a timed tasked based on timeout
        // we cannot access the future interface directly, so we need to create a new one
        // with the same lifetime
        auto fi = new QFutureInterface<void>();
        auto watcher = new QFutureWatcher<void>();
        connect(watcher, &QFutureWatcherBase::finished, [fi, watcher] {
            fi->reportFinished();
            delete fi;
            watcher->deleteLater();
        });
        watcher->setFuture(future);
        ProgressManager::addTimedTask(*fi, name, id, qMax(2, timeoutS() / 5)/*itsmagic*/);
    }
}

void ShellCommand::coreAboutToClose()
{
    abort();
}

} // namespace Core
