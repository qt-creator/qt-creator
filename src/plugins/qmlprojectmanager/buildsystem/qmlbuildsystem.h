// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmlprojectmanager_global.h"

#include <projectexplorer/buildsystem.h>
#include <utils/filesystemwatcher.h>

#include "qmlprojectmanager/qmlprojectexporter/exporter.h"

namespace QmlProjectManager {

class QmlProject;
class QmlProjectItem;
class QmlProjectFile;

class QMLPROJECTMANAGER_EXPORT QmlBuildSystem final : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit QmlBuildSystem(ProjectExplorer::BuildConfiguration *bc);
    ~QmlBuildSystem() = default;

    void triggerParsing() final;

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;
    bool addFiles(ProjectExplorer::Node *context,
                  const Utils::FilePaths &filePaths,
                  Utils::FilePaths *notAdded = nullptr) override;
    bool deleteFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths) override;
    bool renameFiles(ProjectExplorer::Node *context,
                     const Utils::FilePairs &filesToRename,
                     Utils::FilePaths *notRenamed) override;

    bool updateProjectFile();

    QmlProject *qmlProject() const;

    QVariant additionalData(Utils::Id id) const override;

    enum class RefreshOptions {
        NoFileRefresh,
        Files,
        Project,
    };

    void refresh(RefreshOptions options);

    bool setMainFileInProjectFile(const Utils::FilePath &newMainFilePath);
    bool setMainUiFileInProjectFile(const Utils::FilePath &newMainUiFilePath);
    bool setMainUiFileInMainFile(const Utils::FilePath &newMainUiFilePath);

    Utils::FilePath canonicalProjectDir() const;
    QString mainFile() const;
    QString mainUiFile() const;
    Utils::FilePath mainFilePath() const;
    Utils::FilePath mainUiFilePath() const;

    bool qtForMCUs() const;
    bool qt6Project() const;

    Utils::FilePath targetDirectory() const;
    Utils::FilePath targetFile(const Utils::FilePath &sourceFile) const;

    Utils::EnvironmentItems environment() const;

    QStringList allImports() const;
    QStringList mockImports() const;
    QStringList absoluteImportPaths() const;
    QStringList targetImportPaths() const;
    QStringList fileSelectors() const;

    QStringList importPaths() const;
    void addImportPath(const Utils::FilePath &path);

    bool multilanguageSupport() const;
    QStringList supportedLanguages() const;
    void setSupportedLanguages(QStringList languages);

    QString primaryLanguage() const;
    void setPrimaryLanguage(QString language);

    bool enableCMakeGeneration() const;
    void setEnableCMakeGeneration(bool enable);

    bool enablePythonGeneration() const;
    void setEnablePythonGeneration(bool enable);

    bool standaloneApp() const;
    void setStandaloneApp(bool enable);

    bool forceFreeType() const;
    bool widgetApp() const;

    QStringList shaderToolArgs() const;
    QStringList shaderToolFiles() const;

    QString versionQt() const;
    QString versionQtQuick() const;
    QString versionDesignStudio() const;

    bool addFiles(const QStringList &filePaths);
    void refreshProjectFile();

    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);

    bool blockFilesUpdate() const;
    void setBlockFilesUpdate(bool newBlockFilesUpdate);

    Utils::FilePath getStartupQmlFileWithFallback() const;

    static QmlBuildSystem *getStartupBuildSystem();

    void addQmlProjectModule(const Utils::FilePath &path);

signals:
    void projectChanged();

private:
    bool setFileSettingInProjectFile(const QString &setting,
                                     const Utils::FilePath &mainFilePath,
                                     const QString &oldFile);

    void updateQmlCodeModelInfo(ProjectExplorer::QmlCodeModelInfo &projectInfo) final;

    // this is the main project item
    QSharedPointer<QmlProjectItem> m_projectItem;
    // these are the mcu project items which can be found in the project tree
    QList<QSharedPointer<QmlProjectItem>> m_mcuProjectItems;
    Utils::FileSystemWatcher m_mcuProjectFilesWatcher;
    bool m_blockFilesUpdate = false;

    void initProjectItem();
    void initMcuProjectItems();
    void parseProjectFiles();
    void generateProjectTree();

    void registerMenuButtons();
    void updateDeploymentData();
    friend class FilesUpdateBlocker;

    QmlProjectExporter::Exporter* m_fileGen;
};

void setupQmlBuildConfiguration();

} // namespace QmlProjectManager
