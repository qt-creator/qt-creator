/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include "toolwrapper.h"

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
        QProcess process;
        process.start(toolPath.toString(), {"--version"});
        if (process.waitForFinished()) {
            return Version::fromString(QString::fromUtf8(process.readLine()));
        }
    }
    return {};
}

Utils::optional<Utils::FilePath> ToolWrapper::findTool(const QStringList &exeNames)
{
    using namespace Utils;
    Environment systemEnvironment = Environment::systemEnvironment();
    for (const auto &exe : exeNames) {
        const FilePath exe_path = systemEnvironment.searchInPath(exe);
        if (exe_path.exists())
            return exe_path;
    }
    return Utils::nullopt;
}

} // namespace Internal
} // namespace MesonProjectManager
