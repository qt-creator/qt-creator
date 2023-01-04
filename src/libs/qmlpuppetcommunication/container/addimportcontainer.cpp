// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "addimportcontainer.h"

#include <QDebug>
#include <QDataStream>

namespace QmlDesigner {

AddImportContainer::AddImportContainer() = default;

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
