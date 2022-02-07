/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprojectmanager_global.h"
#include "qmlprojectnodes.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>

#include <utils/environment.h>

#include <QPointer>

namespace QmlProjectManager {

class QmlProject;
class QmlProjectItem;

class QMLPROJECTMANAGER_EXPORT QmlBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit QmlBuildSystem(ProjectExplorer::Target *target);
    ~QmlBuildSystem();

    void triggerParsing() final;

    bool supportsAction(ProjectExplorer::Node *context,
                        ProjectExplorer::ProjectAction action,
                        const ProjectExplorer::Node *node) const override;
    bool addFiles(ProjectExplorer::Node *context,
                  const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded = nullptr) override;
    bool deleteFiles(ProjectExplorer::Node *context,
                     const Utils::FilePaths &filePaths) override;
    bool renameFile(ProjectExplorer::Node *context,
                    const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override;
    QString name() const override { return QLatin1String("qml"); }

    QmlProject *qmlProject() const;

    QVariant additionalData(Utils::Id id) const override;

    enum RefreshOption {
        ProjectFile   = 0x01,
        Files         = 0x02,
        Configuration = 0x04,
        Everything    = ProjectFile | Files | Configuration
    };
    Q_DECLARE_FLAGS(RefreshOptions,RefreshOption)

    void refresh(RefreshOptions options);

    Utils::FilePath canonicalProjectDir() const;
    QString mainFile() const;
    Utils::FilePath mainFilePath() const;

    bool qtForMCUs() const;
    bool qt6Project() const;
    void setMainFile(const QString &mainFilePath);
    Utils::FilePath targetDirectory() const;
    Utils::FilePath targetFile(const Utils::FilePath &sourceFile) const;

    Utils::EnvironmentItems environment() const;
    QStringList customImportPaths() const;
    QStringList customFileSelectors() const;
    bool multilanguageSupport() const;
    QStringList supportedLanguages() const;
    void setSupportedLanguages(QStringList languages);
    QString primaryLanguage() const;
    void setPrimaryLanguage(QString language);
    bool forceFreeType() const;
    bool widgetApp() const;

    bool addFiles(const QStringList &filePaths);

    void refreshProjectFile();

    static Utils::FilePath activeMainFilePath();
    static QStringList makeAbsolute(const Utils::FilePath &path, const QStringList &relativePaths);

    void generateProjectTree();
    void updateDeploymentData();
    void refreshFiles(const QSet<QString> &added, const QSet<QString> &removed);
    void refreshTargetDirectory();
    void onActiveTargetChanged(ProjectExplorer::Target *target);
    void onKitChanged();

    // plain format
    void parseProject(RefreshOptions options);

private:
    std::unique_ptr<QmlProjectItem> m_projectItem;
    Utils::FilePath m_canonicalProjectDir;
    bool m_blockFilesUpdate = false;
    friend class FilesUpdateBlocker;
};

class FilesUpdateBlocker {
public:
    FilesUpdateBlocker(QmlBuildSystem* bs): m_bs(bs) {
        if (m_bs)
            m_bs->m_blockFilesUpdate = true;
    }

    ~FilesUpdateBlocker() {
        if (m_bs) {
            m_bs->m_blockFilesUpdate = false;
            m_bs->refresh(QmlBuildSystem::Everything);
        }
    }
private:
    QPointer<QmlBuildSystem> m_bs;
};

class QMLPROJECTMANAGER_EXPORT QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit QmlProject(const Utils::FilePath &filename);

    ProjectExplorer::Tasks projectIssues(const ProjectExplorer::Kit *k) const final;

    static bool isQtDesignStudio();

    bool isEditModePreferred() const override;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    ProjectExplorer::DeploymentKnowledge deploymentKnowledge() const override;
    Utils::FilePaths getUiQmlFilesForFolder(const Utils::FilePath &folder);

    QMetaObject::Connection m_openFileConnection;
};

} // namespace QmlProjectManager

Q_DECLARE_OPERATORS_FOR_FLAGS(QmlProjectManager::QmlBuildSystem::RefreshOptions)
