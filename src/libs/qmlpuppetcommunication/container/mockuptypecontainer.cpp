// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mockuptypecontainer.h"

#include <QDebug>

namespace QmlDesigner {

QmlDesigner::MockupTypeContainer::MockupTypeContainer() = default;

QmlDesigner::MockupTypeContainer::MockupTypeContainer(const QmlDesigner::TypeName &type,
                                                      const QString &importUri,
                                                      int majorVersion,
                                                      int minorVersion, bool isItem)
    : m_typeName(type)
    ,m_importUri(importUri)
    ,m_majorVersion(majorVersion)
    ,m_minorVersion(minorVersion)
    ,m_isItem(isItem)
{

}

QString MockupTypeContainer::importUri() const
{
    return m_importUri;
}

QDataStream &operator<<(QDataStream &out, const MockupTypeContainer &container)
{
    out << container.typeName();
    out << container.importUri();
    out << container.majorVersion();
    out << container.minorVersion();
    out << container.isItem();

    return out;
}

QDataStream &operator>>(QDataStream &in, MockupTypeContainer &container)
{
    in >> container.m_typeName;
    in >> container.m_importUri;
    in >> container.m_majorVersion;
    in >> container.m_minorVersion;
    in >> container.m_isItem;

    return in;
}

QDebug operator <<(QDebug debug, const MockupTypeContainer &container)
{
    return debug.nospace() << "MockupTypeContainer("
                           << "typeName: " << container.typeName() << ", "
                           << "importUri: " << container.importUri() << ", "
                           << "majorVersion: " << container.majorVersion() << ", "
                           << "minorVersion: " << container.minorVersion() << ", "
                           << "isItem: " << container.isItem() << ")";
}


} // namespace QmlDesigner
