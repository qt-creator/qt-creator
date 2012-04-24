/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GENERICPROJECT_H
#define GENERICPROJECT_H

#include "genericprojectmanager.h"
#include "genericprojectnodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace ProjectExplorer { class ToolChain; }

namespace GenericProjectManager {
namespace Internal {

class GenericBuildConfiguration;
class GenericProject;
class GenericTarget;
class GenericMakeStep;
class GenericProjectFile;

class GenericProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    GenericProject(Manager *manager, const QString &filename);
    virtual ~GenericProject();

    QString filesFileName() const;
    QString includesFileName() const;
    QString configFileName() const;

    QString displayName() const;
    Core::Id id() const;
    Core::IDocument *document() const;
    ProjectExplorer::IProjectManager *projectManager() const;

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    GenericProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

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

    QStringList includePaths() const;
    void setIncludePaths(const QStringList &includePaths);

    QByteArray defines() const;
    QStringList allIncludePaths() const;
    QStringList projectIncludePaths() const;
    QStringList files() const;
    QStringList generated() const;

protected:
    bool fromMap(const QVariantMap &map);

private:
    void evaluateBuildSystem();
    bool saveRawFileList(const QStringList &rawFileList);
    void parseProject(RefreshOptions options);
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    Manager *m_manager;
    QString m_fileName;
    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    QString m_projectName;
    GenericProjectFile *m_creatorIDocument;
    GenericProjectFile *m_filesIDocument;
    GenericProjectFile *m_includesIDocument;
    GenericProjectFile *m_configIDocument;
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_generated;
    QStringList m_includePaths;
    QStringList m_projectIncludePaths;
    QByteArray m_defines;

    GenericProjectNode *m_rootNode;
    QFuture<void> m_codeModelFuture;
};

class GenericProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    GenericProjectFile(GenericProject *parent, QString fileName, GenericProject::RefreshOptions options);
    virtual ~GenericProjectFile();

    virtual bool save(QString *errorString, const QString &fileName, bool autoSave);
    virtual QString fileName() const;

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isSaveAsAllowed() const;
    virtual void rename(const QString &newName);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    GenericProject *m_project;
    QString m_fileName;
    GenericProject::RefreshOptions m_options;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECT_H
