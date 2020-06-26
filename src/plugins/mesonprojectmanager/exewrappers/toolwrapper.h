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
#pragma once
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/id.h>
#include <utils/qtcassert.h>
#include <versionhelper.h>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QUuid>
#include <QVariant>
#include <QVariantMap>

namespace MesonProjectManager {
namespace Internal {

class Command
{
    Utils::CommandLine m_cmd;
    Utils::FilePath m_workDir;

public:
    Command() = default;
    Command(const Utils::FilePath &exe, const Utils::FilePath &workDir, const QStringList &args)
        : m_cmd{exe, args}
        , m_workDir{workDir}
    {}
    inline const Utils::CommandLine &cmdLine() const { return m_cmd; }
    inline const Utils::FilePath &workDir() const { return m_workDir; }
    inline Utils::FilePath executable() const { return m_cmd.executable(); }
    inline QStringList arguments() const { return m_cmd.splitArguments(); }
    inline QString toUserOutput() const { return m_cmd.toUserOutput(); };
};

class ToolWrapper
{
public:
    virtual ~ToolWrapper(){};
    ToolWrapper() = delete;
    ToolWrapper(const QString &name, const Utils::FilePath &path, bool autoDetected = false);
    ToolWrapper(const QString &name,
                const Utils::FilePath &path,
                const Utils::Id &id,
                bool autoDetected = false);
    ToolWrapper(const ToolWrapper &other) = default;
    ToolWrapper(ToolWrapper &&other) = default;
    ToolWrapper &operator=(const ToolWrapper &other) = default;
    ToolWrapper &operator=(ToolWrapper &&other) = default;

    inline const Version &version() const noexcept { return m_version; };
    inline bool isValid() const noexcept { return m_isValid; };
    inline bool autoDetected() const noexcept { return m_autoDetected; };
    inline Utils::Id id() const noexcept { return m_id; };
    inline Utils::FilePath exe() const noexcept { return m_exe; };
    inline QString name() const noexcept { return m_name; };

    inline void setName(const QString &newName) { m_name = newName; }
    virtual void setExe(const Utils::FilePath &newExe);

    static Version read_version(const Utils::FilePath &toolPath);

    static Utils::optional<Utils::FilePath> findTool(const QStringList &exeNames);

    template<typename T>
    friend QVariantMap toVariantMap(const T &);
    template<typename T>
    friend T fromVariantMap(const QVariantMap &);

protected:
    Version m_version;
    bool m_isValid;
    bool m_autoDetected;
    Utils::Id m_id;
    Utils::FilePath m_exe;
    QString m_name;
};

template<typename T>
QVariantMap toVariantMap(const T &);
template<typename T>
T fromVariantMap(const QVariantMap &);

} // namespace Internal
} // namespace MesonProjectManager
