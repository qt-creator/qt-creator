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

#include "genericprojectmanager.h"
#include "genericprojectnodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>

namespace GenericProjectManager {
namespace Internal {

class GenericProjectFile;

class GenericProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    GenericProject(Manager *manager, const QString &filename);
    ~GenericProject() override;

    QString filesFileName() const;
    QString includesFileName() const;
    QString configFileName() const;

    QString displayName() const override;
    Manager *projectManager() const override;

    QStringList files(FilesMode fileMode) const override;

    QStringList buildTargets() const;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);
    bool setFiles(const QStringList &filePaths);
    bool renameFile(const QString &filePath, const QString &newFilePath);

    enum RefreshOptions {
        Files         = 0x01,
        Configuration = 0x02,
        Everything    = Files | Configuration
    };

    void refresh(RefreshOptions options);

    QStringList projectIncludePaths() const;
    QStringList files() const;

protected:
    RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parseProject(RefreshOptions options);
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = nullptr) const;

    void refreshCppCodeModel();
    void activeTargetWasChanged();
    void activeBuildConfigurationWasChanged();

    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    QString m_projectName;
    GenericProjectFile *m_filesIDocument;
    GenericProjectFile *m_includesIDocument;
    GenericProjectFile *m_configIDocument;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_rawProjectIncludePaths;
    QStringList m_projectIncludePaths;

    QFuture<void> m_codeModelFuture;

    ProjectExplorer::Target *m_activeTarget = nullptr;
};

class GenericProjectFile : public Core::IDocument
{
public:
    GenericProjectFile(GenericProject *parent, QString fileName, GenericProject::RefreshOptions options);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    GenericProject *m_project;
    GenericProject::RefreshOptions m_options;
};

} // namespace Internal
} // namespace GenericProjectManager
