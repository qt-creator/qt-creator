// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compilationdatabaseutils.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>

#include <texteditor/texteditor.h>

#include <utils/filesystemwatcher.h>

#include <QFutureWatcher>

namespace CppEditor { class CppProjectUpdater; }
namespace ProjectExplorer { class Kit; }
namespace Utils { class FileSystemWatcher; }

namespace CompilationDatabaseProjectManager {
namespace Internal {
class CompilationDbParser;

class CompilationDatabaseProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit CompilationDatabaseProject(const Utils::FilePath &filename);
    Utils::FilePath rootPathFromSettings() const;

private:
    void configureAsExampleProject(ProjectExplorer::Kit *kit) override;
};

class CompilationDatabaseBuildSystem : public ProjectExplorer::BuildSystem
{
public:
    explicit CompilationDatabaseBuildSystem(ProjectExplorer::Target *target);
    ~CompilationDatabaseBuildSystem();

    void triggerParsing() final;
    QString name() const final { return QLatin1String("compilationdb"); }

    void reparseProject();
    void updateDeploymentData();
    void buildTreeAndProjectParts();

    QFutureWatcher<void> m_parserWatcher;
    std::unique_ptr<CppEditor::CppProjectUpdater> m_cppCodeModelUpdater;
    MimeBinaryCache m_mimeBinaryCache;
    QByteArray m_projectFileHash;
    CompilationDbParser *m_parser = nullptr;
    Utils::FileSystemWatcher * const m_deployFileWatcher;
};

class CompilationDatabaseEditorFactory : public TextEditor::TextEditorFactory
{
public:
    CompilationDatabaseEditorFactory();
};

class CompilationDatabaseBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
public:
    CompilationDatabaseBuildConfigurationFactory();
};

} // namespace Internal
} // namespace CompilationDatabaseProjectManager
