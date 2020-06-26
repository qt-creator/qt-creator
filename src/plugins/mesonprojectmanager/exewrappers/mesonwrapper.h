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
#include "../mesonpluginconstants.h"
#include "toolwrapper.h"
#include "utils/environment.h"
#include "utils/fileutils.h"
#include <utils/id.h>
#include "utils/optional.h"
#include <tuple>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>
#include <QTemporaryFile>

namespace MesonProjectManager {
namespace Internal {

template<typename File_t>
bool containsFiles(const QString &path, const File_t &file)
{
    return QFile::exists(QString("%1/%2").arg(path).arg(file));
}

template<typename File_t, typename... T>
bool containsFiles(const QString &path, const File_t &file, const T &... files)
{
    return containsFiles(path, file) && containsFiles(path, files...);
}

inline bool run_meson(const Command &command, QIODevice *output = nullptr)
{
    QProcess process;
    process.setWorkingDirectory(command.workDir().toString());
    process.start(command.executable().toString(), command.arguments());
    if (!process.waitForFinished())
        return false;
    if (output) {
        output->write(process.readAllStandardOutput());
    }
    return process.exitCode() == 0;
}

inline bool isSetup(const Utils::FilePath &buildPath)
{
    using namespace Utils;
    return containsFiles(buildPath.pathAppended(Constants::MESON_INFO_DIR).toString(),
                                  Constants::MESON_INTRO_TESTS,
                                  Constants::MESON_INTRO_TARGETS,
                                  Constants::MESON_INTRO_INSTALLED,
                                  Constants::MESON_INTRO_BENCHMARKS,
                                  Constants::MESON_INTRO_BUIDOPTIONS,
                                  Constants::MESON_INTRO_PROJECTINFO,
                                  Constants::MESON_INTRO_DEPENDENCIES,
                                  Constants::MESON_INTRO_BUILDSYSTEM_FILES);
}

class MesonWrapper final : public ToolWrapper
{
public:
    using ToolWrapper::ToolWrapper;

    Command setup(const Utils::FilePath &sourceDirectory,
                  const Utils::FilePath &buildDirectory,
                  const QStringList &options = {}) const;
    Command configure(const Utils::FilePath &sourceDirectory,
                      const Utils::FilePath &buildDirectory,
                      const QStringList &options = {}) const;

    Command regenerate(const Utils::FilePath &sourceDirectory,
                       const Utils::FilePath &buildDirectory)const;

    Command introspect(const Utils::FilePath &sourceDirectory) const;

    static inline Utils::optional<Utils::FilePath> find()
    {
        return ToolWrapper::findTool({"meson.py","meson"});
    }

    static inline QString toolName() { return {"Meson"}; };
};

template<>
inline QVariantMap toVariantMap<MesonWrapper>(const MesonWrapper &meson)
{
    QVariantMap data;
    data.insert(Constants::ToolsSettings::NAME_KEY, meson.m_name);
    data.insert(Constants::ToolsSettings::EXE_KEY, meson.m_exe.toVariant());
    data.insert(Constants::ToolsSettings::AUTO_DETECTED_KEY, meson.m_autoDetected);
    data.insert(Constants::ToolsSettings::ID_KEY, meson.m_id.toSetting());
    data.insert(Constants::ToolsSettings::TOOL_TYPE_KEY, Constants::ToolsSettings::TOOL_TYPE_MESON);
    return data;
}
template<>
inline MesonWrapper *fromVariantMap<MesonWrapper *>(const QVariantMap &data)
{
    return new MesonWrapper(data[Constants::ToolsSettings::NAME_KEY].toString(),
                            Utils::FilePath::fromVariant(data[Constants::ToolsSettings::EXE_KEY]),
                            Utils::Id::fromSetting(data[Constants::ToolsSettings::ID_KEY]),
                            data[Constants::ToolsSettings::AUTO_DETECTED_KEY].toBool());
}

} // namespace Internal
} // namespace MesonProjectManager
