// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "osparser.h"

#include "projectexplorertr.h"
#include "task.h"

#include <utils/hostosinfo.h>

namespace ProjectExplorer {

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
                scheduleTask(CompileTask(Task::Error, Tr::tr(
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

} // ProjectExplorer
