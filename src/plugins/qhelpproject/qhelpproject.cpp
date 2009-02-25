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

#include "qhelpproject.h"
#include "qhelpprojectmanager.h"
#include "qhelpprojectitems.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <qhelpsystem.h>

using namespace ProjectExplorer;
using namespace QHelpProjectPlugin::Internal;

QHelpProject::QHelpProject(QHelpProjectManager *manager)
{
    m_manager = manager;
    
    ProjectExplorerInterface *projectExplorer = m_manager->projectExplorer();
    projectExplorer->addProjectItem(this);

    m_filesFolder = new QHelpProjectFolder();
    m_filesFolder->setName(QObject::tr("Files"));
    projectExplorer->addProjectItem(m_filesFolder, this);
}

QHelpProject::~QHelpProject()
{

}

bool QHelpProject::open(const QString &fileName)
{
    m_projectFile = fileName;

    QHelpProjectParser parser;
    if (!parser.parse(m_projectFile))
        return false;
    
    m_name = parser.namespaceURI();
    m_files = parser.files();

    QFileInfo fi(fileName);
    QString dir = fi.absolutePath() + QDir::separator();

    ProjectExplorerInterface *projectExplorer = m_manager->projectExplorer();
    QHelpProjectFile *file;
    foreach (QString f, m_files) {
        file = new QHelpProjectFile(dir + f);
        projectExplorer->addProjectItem(file, m_filesFolder);
    }

    m_manager->projectExplorer()->updateItem(this);

    return true;
}

bool QHelpProject::save(const QString &fileName)
{
    return true;
}

QString QHelpProject::fileName() const
{
    return m_projectFile;
}

QString QHelpProject::defaultPath() const
{
    return QString();
}

QString QHelpProject::suggestedFileName() const
{
    return QString();
}

QString QHelpProject::fileFilter() const
{
    return QString();
}

QString QHelpProject::fileExtension() const
{
    return QString();
}

bool QHelpProject::isModified() const
{
    return false;
}

bool QHelpProject::isReadOnly() const
{
    return false;
}

bool QHelpProject::isSaveAsAllowed() const
{
    return true;
}

void QHelpProject::modified(QWorkbench::FileInterface::ReloadBehavior *behavior)
{

}

IProjectManager *QHelpProject::projectManager() const
{
    return m_manager;
}

QVariant QHelpProject::projectProperty(const QString &key) const
{
    return QVariant();
}

void QHelpProject::setProjectProperty(const QString &key, const QVariant &value) const
{

}

void QHelpProject::executeProjectCommand(int cmd)
{

}

bool QHelpProject::supportsProjectCommand(int cmd)
{
    return false;
}

bool QHelpProject::hasProjectProperty(ProjectProperty property) const
{
    return false;
}

QList<QWorkbench::FileInterface*> QHelpProject::dependencies()
{
    return QList<QWorkbench::FileInterface*>();
}

void QHelpProject::setExtraApplicationRunArguments(const QStringList &args)
{

}

void QHelpProject::setCustomApplicationOutputHandler(QObject *handler)
{

}

QStringList QHelpProject::files(const QList<QRegExp> &fileMatcher)
{
    return m_files;
}

ProjectItemInterface::ProjectItemKind QHelpProject::kind() const
{
    return ProjectItemInterface::Project;
}

QString QHelpProject::name() const
{
    return m_name;
}

QIcon QHelpProject::icon() const
{
    return QIcon();
}
