/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
