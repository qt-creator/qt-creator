/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QObject>

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace ClangTools {
namespace Internal {

// TODO: Incorporate Clang Static Analyzer's ProjectSettings
class ClangToolsProjectSettings : public QObject
{
    Q_OBJECT

public:
    ClangToolsProjectSettings(ProjectExplorer::Project *project);
    ~ClangToolsProjectSettings() override;

    QSet<Utils::FileName> selectedDirs() const { return m_selectedDirs; }
    void setSelectedDirs(const QSet<Utils::FileName> &value) { m_selectedDirs = value; }

    QSet<Utils::FileName> selectedFiles() const { return m_selectedFiles; }
    void setSelectedFiles(const QSet<Utils::FileName> &value) { m_selectedFiles = value; }

private:
    void load();
    void store();

    ProjectExplorer::Project *m_project;
    QSet<Utils::FileName> m_selectedDirs;
    QSet<Utils::FileName> m_selectedFiles;
};

// TODO: Incorporate Clang Static Analyzer's ProjectSettingsManager
class ClangToolsProjectSettingsManager
{
public:
    ClangToolsProjectSettingsManager();

    static ClangToolsProjectSettings *getSettings(ProjectExplorer::Project *project);

private:
    static void handleProjectToBeRemoved(ProjectExplorer::Project *project);

    using SettingsMap = QHash<ProjectExplorer::Project *, QSharedPointer<ClangToolsProjectSettings>>;
    static SettingsMap m_settings;
};

} // namespace Internal
} // namespace ClangTools
