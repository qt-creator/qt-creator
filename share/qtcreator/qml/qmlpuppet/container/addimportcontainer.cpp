/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "addimportcontainer.h"

#include <QDebug>
#include <QDataStream>

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

QDebug operator <<(QDebug debug, const AddImportContainer &container)
{
    debug.nospace() << "AddImportContainer(";

    if (!container.url().isEmpty())
        debug.nospace() << "url: " << container.url() << ", ";

    if (!container.fileName().isEmpty())
        debug.nospace() << "fileName: " << container.fileName() << ", ";

    if (!container.version().isEmpty())
        debug.nospace()  << "version: " << container.version() << ", ";

    if (!container.alias().isEmpty())
        debug.nospace()  << "alias: " << container.alias() << ", ";

    debug.nospace() << "importPaths: " << container.importPaths();

    return debug.nospace() << ")";
}

} // namespace QmlDesigner
