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

#include "compilationdatabaseutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <texteditor/texteditor.h>
#include <utils/filesystemwatcher.h>

#include <QFutureWatcher>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace CppTools {
class CppProjectUpdater;
}

namespace ProjectExplorer {
class Kit;
}

namespace CompilationDatabaseProjectManager {
namespace Internal {
class CompilationDbParser;

class CompilationDatabaseProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CompilationDatabaseProject(const Utils::FilePath &filename);
    ~CompilationDatabaseProject() override;
    bool needsConfiguration() const override { return false; }

private:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

    void reparseProject();
    void buildTreeAndProjectParts();
    Utils::FilePath rootPathFromSettings() const;

    QFutureWatcher<void> m_parserWatcher;
    std::unique_ptr<CppTools::CppProjectUpdater> m_cppCodeModelUpdater;
    std::unique_ptr<ProjectExplorer::Kit> m_kit;
    MimeBinaryCache m_mimeBinaryCache;
    QTimer * const m_parseDelay;
    CompilationDbParser *m_parser = nullptr;
};

class CompilationDatabaseEditorFactory : public TextEditor::TextEditorFactory
{
    Q_OBJECT

public:
    CompilationDatabaseEditorFactory();
};

class CompilationDatabaseBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
public:
    CompilationDatabaseBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    ProjectExplorer::NamedWidget *createConfigWidget() override;

protected:
    void initialize() override;
};

class CompilationDatabaseBuildConfigurationFactory
    : public ProjectExplorer::BuildConfigurationFactory
{
    Q_OBJECT
public:
    CompilationDatabaseBuildConfigurationFactory();

    QList<ProjectExplorer::BuildInfo> availableBuilds(const ProjectExplorer::Kit *k,
                                                      const Utils::FilePath &projectPath,
                                                      bool forSetup) const override;
};

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
