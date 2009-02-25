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

#include "qhelpprojectitems.h"

#include <QtGui/QFileIconProvider>

using namespace ProjectExplorer;
using namespace QHelpProjectPlugin::Internal;

QIcon QHelpProjectFile::m_icon = QIcon();
QIcon QHelpProjectFolder::m_icon = QIcon();

QHelpProjectFile::QHelpProjectFile(const QString &fileName)
{
    m_file = fileName;

    if (m_icon.isNull()) {
        QFileIconProvider iconProvider;
        m_icon = iconProvider.icon(QFileIconProvider::File);
    }
}

QString QHelpProjectFile::fullName() const
{
    return m_file;
}

ProjectItemInterface::ProjectItemKind QHelpProjectFile::kind() const
{
    return ProjectItemInterface::ProjectFile;
}

QString QHelpProjectFile::name() const
{
    QFileInfo fi(m_file);
    return fi.fileName();
}

QIcon QHelpProjectFile::icon() const
{
    return m_icon;
}



QHelpProjectFolder::QHelpProjectFolder()
{
    if (m_icon.isNull()) {
        QFileIconProvider iconProvider;
        m_icon = iconProvider.icon(QFileIconProvider::Folder);
    }
}

QHelpProjectFolder::~QHelpProjectFolder()
{
}

void QHelpProjectFolder::setName(const QString &name)
{
    m_name = name;
}

ProjectItemInterface::ProjectItemKind QHelpProjectFolder::kind() const
{
    return ProjectItemInterface::ProjectFolder;
}

QString QHelpProjectFolder::name() const
{
    return m_name;
}

QIcon QHelpProjectFolder::icon() const
{
    return m_icon;
}
