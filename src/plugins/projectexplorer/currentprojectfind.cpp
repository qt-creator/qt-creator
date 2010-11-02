/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "currentprojectfind.h"

#include "projectexplorer.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind(ProjectExplorerPlugin *plugin, SearchResultWindow *resultWindow)
  : AllProjectsFind(plugin, resultWindow),
    m_plugin(plugin)
{
    connect(m_plugin, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SIGNAL(changed()));
}

QString CurrentProjectFind::id() const
{
    return QLatin1String("Current Project");
}

QString CurrentProjectFind::displayName() const
{
    return tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return m_plugin->currentProject() != 0 && BaseFileFind::isEnabled();
}

QList<Project *> CurrentProjectFind::projects() const
{
    return QList<Project *>() << m_plugin->currentProject();
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
