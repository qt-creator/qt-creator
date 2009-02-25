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

#include "qhelpprojectmanager.h"
#include "qhelpproject.h"

#include <QtCore/qplugin.h>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace QHelpProjectPlugin::Internal;

QHelpProjectManager::QHelpProjectManager()
{
}

QHelpProjectManager::~QHelpProjectManager()
{
}

bool QHelpProjectManager::init(ExtensionSystem::PluginManager *pm, QString *error_message)
{
    m_pm = pm;
    m_core = m_pm->interface<QWorkbench::ICore>();
    QWorkbench::ActionManager *am = m_core->actionManager();

    m_projectContext = m_core->uniqueIDManager()->
        uniqueIdentifier(QLatin1String("QHelpProject"));

    m_languageId = m_core->uniqueIDManager()->
        uniqueIdentifier(QLatin1String("QHelpLanguage"));

    m_projectExplorer = m_pm->interface<ProjectExplorer::ProjectExplorerInterface>();

    return true;
}

void QHelpProjectManager::extensionsInitialized()
{
}

void QHelpProjectManager::cleanup()
{
}

int QHelpProjectManager::projectContext() const
{
    return m_projectContext;
}

int QHelpProjectManager::projectLanguage() const
{
    return m_languageId;
}

bool QHelpProjectManager::canOpen(const QString &fileName)
{
    qDebug() << "can open " << fileName;
    QFileInfo fi(fileName);
    if (fi.suffix() == QLatin1String("qthp"))
        return true;
    return false;
}

QList<ProjectExplorer::Project*> QHelpProjectManager::open(const QString &fileName)
{
    QList<ProjectExplorer::Project*> lst;
    QHelpProject *pro = new QHelpProject(this);
    if (pro->open(fileName)) {
        lst.append(pro);
    } else {
        delete pro;
    }
    return lst;
}

QString QHelpProjectManager::fileFilter() const
{
    return tr("Qt Help Project File (*.qthp)");
}

QVariant QHelpProjectManager::editorProperty(const QString &key) const
{
    return QVariant();
}

ProjectExplorer::ProjectExplorerInterface *QHelpProjectManager::projectExplorer() const
{
    return m_projectExplorer;
}

Q_EXPORT_PLUGIN(QHelpProjectManager)
