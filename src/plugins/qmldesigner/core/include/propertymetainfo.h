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

#ifndef PROPERTYMETAINFO_H
#define PROPERTYMETAINFO_H

#include "corelib_global.h"

#include <QString>
#include <QExplicitlySharedDataPointer>
#include <QVariant>

#include <enumeratormetainfo.h>

namespace QmlDesigner {

class AbstractProperty;
class MetaInfo;
class NodeMetaInfo;

namespace Internal {

class MetaInfoPrivate;
class MetaInfoParser;
class PropertyMetaInfoData;
class ObjectNodeInstance;
class SubComponentManagerPrivate;

}

class CORESHARED_EXPORT PropertyMetaInfo
{
    friend class QmlDesigner::Internal::MetaInfoPrivate;
    friend class QmlDesigner::Internal::MetaInfoParser;
    friend class QmlDesigner::Internal::ObjectNodeInstance;
    friend class QmlDesigner::Internal::SubComponentManagerPrivate;
    friend class QmlDesigner::MetaInfo;
    friend class QmlDesigner::AbstractProperty;
    friend class QmlDesigner::NodeMetaInfo;
public:
    PropertyMetaInfo();
    ~PropertyMetaInfo();

    PropertyMetaInfo(const PropertyMetaInfo &other);
    PropertyMetaInfo& operator=(const PropertyMetaInfo &other);

    bool isValid() const;

    QString name() const;
    QString type() const;

    QVariant::Type variantTypeId() const;

    bool isReadable() const;
    bool isWriteable() const;
    bool isResettable() const;
    bool isValueType() const;
    bool isListProperty() const;

    bool isEnumType() const;
    bool isFlagType() const;

    QVariant defaultValue(const NodeMetaInfo &nodeMetaInfo) const;
    bool isVisibleToPropertyEditor() const;

    const EnumeratorMetaInfo enumerator() const;

    QVariant castedValue(const QVariant &variant) const;

private:
    void setName(const QString &name);
    void setType(const QString &type);
    void setValid(bool isValid);

    void setReadable(bool isReadable);
    void setWritable(bool isWritable);
    void setResettable(bool isRessetable);

    void setEnumType(bool isEnumType);
    void setFlagType(bool isFlagType);

    void setDefaultValue(const NodeMetaInfo &nodeMetaInfo, const QVariant &value);
    void setIsVisibleToPropertyEditor(bool isVisible);

    void setEnumerator(const EnumeratorMetaInfo &info);

    bool hasDotSubProperties() const;


private:
    QExplicitlySharedDataPointer<Internal::PropertyMetaInfoData> m_data;
};

}


#endif // PROPERTYMETAINFO_H
