/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmldesignercorelib_global.h>

#include <QSharedPointer>
#include <QString>

#include <vector>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT PropertyMetaInfo
{
public:
    PropertyMetaInfo(QSharedPointer<class NodeMetaInfoPrivate> nodeMetaInfoPrivateData,
                     const PropertyName &propertyName);
    ~PropertyMetaInfo();

    const TypeName &propertyTypeName() const;
    class NodeMetaInfo propertyNodeMetaInfo() const;

    bool isWritable() const;
    bool isListProperty() const;
    bool isEnumType() const;
    bool isPrivate() const;
    bool isPointer() const;
    QVariant castedValue(const QVariant &value) const;
    const PropertyName &name() const & { return m_propertyName; }

    template<typename... TypeName>
    bool hasPropertyTypeName(const TypeName &...typeName) const
    {
        auto propertyTypeName_ = propertyTypeName();
        return ((propertyTypeName_ == typeName) || ...);
    }

    template<typename... TypeName>
    bool hasPropertyTypeName(const std::tuple<TypeName...> &typeNames) const
    {
        return std::apply([&](auto... typeName) { return hasPropertyTypeName(typeName...); },
                          typeNames);
    }

    bool propertyTypeNameIsColor() const { return hasPropertyTypeName("QColor", "color"); }
    bool propertyTypeNameIsString() const { return hasPropertyTypeName("QString", "string"); }
    bool propertyTypeNameIsUrl() const { return hasPropertyTypeName("QUrl", "url"); }

private:
    QSharedPointer<class NodeMetaInfoPrivate> m_nodeMetaInfoPrivateData;
    PropertyName m_propertyName;
};

using PropertyMetaInfos = std::vector<PropertyMetaInfo>;

} // namespace QmlDesigner
