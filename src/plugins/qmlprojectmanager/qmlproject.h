/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QMLPROJECT_H
#define QMLPROJECT_H

#include "qmlprojectmanager.h"
#include "qmlprojectnodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <coreplugin/ifile.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QStringListModel;
QT_END_NAMESPACE

namespace Core {
namespace Utils {
class PathChooser;
}
}

namespace QmlProjectManager {
namespace Internal {

class QmlMakeStep;
class QmlProjectFile;

class QmlProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    QmlProject(Manager *manager, const QString &filename);
    virtual ~QmlProject();

    QString filesFileName() const;

    virtual QString name() const;
    virtual Core::IFile *file() const;
    virtual ProjectExplorer::IProjectManager *projectManager() const;

    virtual QList<ProjectExplorer::Project *> dependsOn();

    virtual bool isApplication() const;

    virtual ProjectExplorer::Environment environment(const QString &buildConfiguration) const;
    virtual QString buildDirectory(const QString &buildConfiguration) const;

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual QList<ProjectExplorer::BuildStepConfigWidget*> subConfigWidgets();

    virtual void newBuildConfiguration(const QString &buildConfiguration);
    virtual QmlProjectNode *rootProjectNode() const;
    virtual QStringList files(FilesMode fileMode) const;

    QStringList targets() const;
    QmlMakeStep *makeStep() const;
    QString buildParser(const QString &buildConfiguration) const;

    enum RefreshOptions {
        Files         = 0x01,
        Configuration = 0x02,
        Everything    = Files | Configuration
    };

    void refresh(RefreshOptions options);

    QStringList files() const;

protected:
    virtual void saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer);
    virtual void restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader);

private:
    void parseProject(RefreshOptions options);
    QStringList convertToAbsoluteFiles(const QStringList &paths) const;

    Manager *m_manager;
    QString m_fileName;
    QString m_filesFileName;
    QmlProjectFile *m_file;
    QString m_projectName;

    QStringList m_files;

    QmlProjectNode *m_rootNode;
};

class QmlProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    QmlProjectFile(QmlProject *parent, QString fileName);
    virtual ~QmlProjectFile();

    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    virtual void modified(ReloadBehavior *behavior);

private:
    QmlProject *m_project;
    QString m_fileName;
};

class QmlBuildSettingsWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    QmlBuildSettingsWidget(QmlProject *project);
    virtual ~QmlBuildSettingsWidget();

    virtual QString displayName() const;

    virtual void init(const QString &buildConfiguration);

private Q_SLOTS:
    void buildDirectoryChanged();

private:
    QmlProject *m_project;
    Core::Utils::PathChooser *m_pathChooser;
    QString m_buildConfiguration;
};

} // namespace Internal
} // namespace QmlProjectManager

#endif // QMLPROJECT_H
