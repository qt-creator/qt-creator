// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolwrapper.h"

#include <utils/process.h>

#include <QUuid>

namespace MesonProjectManager {
namespace Internal {

ToolWrapper::ToolWrapper(const QString &name, const Utils::FilePath &path, bool autoDetected)
    : m_version(read_version(path))
    , m_isValid{path.exists() && m_version.isValid}
    , m_autoDetected{autoDetected}
    , m_id{Utils::Id::fromString(QUuid::createUuid().toString())}
    , m_exe{path}
    , m_name{name}
{}

ToolWrapper::ToolWrapper(const QString &name,
                         const Utils::FilePath &path,
                         const Utils::Id &id,
                         bool autoDetected)
    : m_version(read_version(path))
    , m_isValid{path.exists() && m_version.isValid}
    , m_autoDetected{autoDetected}
    , m_id{id}
    , m_exe{path}
    , m_name{name}
{
    QTC_ASSERT(m_id.isValid(), m_id = Utils::Id::fromString(QUuid::createUuid().toString()));
}

void ToolWrapper::setExe(const Utils::FilePath &newExe)
{
    m_exe = newExe;
    m_version = read_version(m_exe);
}

Version ToolWrapper::read_version(const Utils::FilePath &toolPath)
{
    if (toolPath.toFileInfo().isExecutable()) {
        Utils::Process process;
        process.setCommand({ toolPath, { "--version" } });
        process.start();
        if (process.waitForFinished())
            return Version::fromString(process.cleanedStdOut());
    }
    return {};
}

std::optional<Utils::FilePath> ToolWrapper::findTool(const QStringList &exeNames)
{
    using namespace Utils;
    Environment systemEnvironment = Environment::systemEnvironment();
    for (const auto &exe : exeNames) {
        const FilePath exe_path = systemEnvironment.searchInPath(exe);
        if (exe_path.exists())
            return exe_path;
    }
    return std::nullopt;
}

} // namespace Internal
} // namespace MesonProjectManager
