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

#include "osparser.h"
#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/hostosinfo.h>

using namespace ProjectExplorer;

OsParser::OsParser()
{
    setObjectName(QLatin1String("OsParser"));
}

Utils::OutputLineParser::Result OsParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    if (type == Utils::StdOutFormat) {
        if (Utils::HostOsInfo::isWindowsHost()) {
            const QString trimmed = line.trimmed();
            if (trimmed == QLatin1String("The process cannot access the file because it is "
                                         "being used by another process.")) {
                scheduleTask(CompileTask(Task::Error, tr(
                       "The process cannot access the file because it is being used "
                       "by another process.\n"
                       "Please close all running instances of your application before "
                       "starting a build.")), 1);
                m_hasFatalError = true;
                return Status::Done;
            }
        }
        return Status::NotHandled;
    }
    if (Utils::HostOsInfo::isLinuxHost()) {
        const QString trimmed = line.trimmed();
        if (trimmed.contains(QLatin1String(": error while loading shared libraries:"))) {
            scheduleTask(CompileTask(Task::Error, trimmed), 1);
            return Status::Done;
        }
    }
    return Status::NotHandled;
}
