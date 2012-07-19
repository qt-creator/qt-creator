/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef ALLPROJECTSFIND_H
#define ALLPROJECTSFIND_H

#include <texteditor/basefilefind.h>

#include <QPointer>

namespace ProjectExplorer {

class ProjectExplorerPlugin;
class Project;

namespace Internal {

class AllProjectsFind : public TextEditor::BaseFileFind
{
    Q_OBJECT

public:
    AllProjectsFind(ProjectExplorerPlugin *plugin);

    QString id() const;
    QString displayName() const;

    bool isEnabled() const;

    QWidget *createConfigWidget();
    void writeSettings(QSettings *settings);
    void readSettings(QSettings *settings);

protected:
    Utils::FileIterator *files(const QStringList &nameFilters,
                               const QVariant &additionalParameters) const;
    Utils::FileIterator *filesForProjects(const QStringList &nameFilters,
                               const QList<Project *> &projects) const;

    QVariant additionalParameters() const;
    QString label() const;
    QString toolTip() const;

private slots:
    void handleFileListChanged();

private:
    ProjectExplorerPlugin *m_plugin;
    QPointer<QWidget> m_configWidget;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // ALLPROJECTSFIND_H
