/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addimportcontainer.h"

namespace QmlDesigner {

AddImportContainer::AddImportContainer()
{
}

AddImportContainer::AddImportContainer(const QUrl &url, const QString &fileName, const QString &version, const QString &alias, const QStringList &importPathList)
    : m_url(url),
      m_fileName(fileName),
      m_version(version),
      m_alias(alias),
      m_importPathList(importPathList)
{
}

QUrl AddImportContainer::url() const
{
    return m_url;
}

QString AddImportContainer::fileName() const
{
    return m_fileName;
}

QString AddImportContainer::version() const
{
    return m_version;
}

QString AddImportContainer::alias() const
{
    return m_alias;
}

QStringList AddImportContainer::importPaths() const
{
    return m_importPathList;
}

QDataStream &operator<<(QDataStream &out, const AddImportContainer &command)
{
    out << command.url();
    out << command.fileName();
    out << command.version();
    out << command.alias();
    out << command.importPaths();

    return out;
}

QDataStream &operator>>(QDataStream &in, AddImportContainer &command)
{
    in >> command.m_url;
    in >> command.m_fileName;
    in >> command.m_version;
    in >> command.m_alias;
    in >> command.m_importPathList;

    return in;
}

} // namespace QmlDesigner
