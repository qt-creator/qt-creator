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

#ifndef QHELPPROJECTITEMS_H
#define QHELPPROJECTITEMS_H

#include <projectexplorer/ProjectExplorerInterfaces>

#include <QtGui/QIcon>

namespace QHelpProjectPlugin {
namespace Internal {

class QHelpProjectFile : public QObject,
    public ProjectExplorer::ProjectFileInterface
{
    Q_OBJECT
    Q_INTERFACES(ProjectExplorer::ProjectFileInterface ProjectExplorer::ProjectItemInterface)

public:
    QHelpProjectFile(const QString &fileName);
    QString fullName() const;

    //ProjectExplorer::ProjectItemInterface
    ProjectItemKind kind() const;

    QString name() const;
    QIcon icon() const;

private:
    QString m_file;
    static QIcon m_icon;
};

class QHelpProjectFolder : public QObject,
    public ProjectExplorer::ProjectFolderInterface
{
    Q_OBJECT
    Q_INTERFACES(ProjectExplorer::ProjectFolderInterface ProjectExplorer::ProjectItemInterface)

public:
    QHelpProjectFolder();
    ~QHelpProjectFolder();

    void setName(const QString &name);
    
    //ProjectExplorer::ProjectItemInterface
    ProjectItemKind kind() const;

    QString name() const;
    QIcon icon() const;

private:
    QString m_name;
    static QIcon m_icon;
};

} // namespace Internal
} // namespace QHelpProject

#endif // QHELPPROJECTITEMS_H
