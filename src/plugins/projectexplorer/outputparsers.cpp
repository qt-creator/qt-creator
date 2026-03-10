// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "outputparsers.h"

#include "ioutputparser.h"
#include "projectexplorertr.h"
#include "task.h"

#include <utils/hostosinfo.h>

namespace ProjectExplorer {
namespace Internal {

class OsParser : public ProjectExplorer::OutputTaskParser
{
public:
    OsParser() { setObjectName(QLatin1String("OsParser")); }

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override
    {
        if (type == Utils::StdOutFormat) {
            if (Utils::HostOsInfo::isWindowsHost()) {
                const QString trimmed = line.trimmed();
                if (trimmed
                    == QLatin1String(
                        "The process cannot access the file because it is "
                        "being used by another process.")) {
                    scheduleTask(
                        CompileTask(
                            Task::Error,
                            Tr::tr(
                                "The process cannot access the file because it is being used "
                                "by another process.\n"
                                "Please close all running instances of your application before "
                                "starting a build.")),
                        1);
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

    bool hasFatalErrors() const override { return m_hasFatalError; }

    bool m_hasFatalError = false;
};

} // namespace Internal

Utils::OutputLineParser *createOsOutputParser()
{
    return new Internal::OsParser;
}

} // namespace ProjectExplorer
