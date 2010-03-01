/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ItemLibraryINFO_H
#define ItemLibraryINFO_H

#include "corelib_global.h"

#include <QExplicitlySharedDataPointer>
#include <QList>
#include <QString>
#include <QIcon>
#include "propertycontainer.h"
#include <qdeclarative.h>

namespace QmlDesigner {

class NodeMetaInfo;

namespace Internal {

class ItemLibraryInfoData;
}

class ItemLibraryInfo;

CORESHARED_EXPORT QDataStream& operator<<(QDataStream& stream, const ItemLibraryInfo& ItemLibraryInfo);
CORESHARED_EXPORT QDataStream& operator>>(QDataStream& stream, ItemLibraryInfo& ItemLibraryInfo);

class CORESHARED_EXPORT ItemLibraryInfo
{
    friend class QmlDesigner::MetaInfo;
    friend class QmlDesigner::Internal::MetaInfoParser;
    friend CORESHARED_EXPORT QDataStream& QmlDesigner::operator<<(QDataStream& stream, const ItemLibraryInfo& image);
    friend CORESHARED_EXPORT QDataStream& QmlDesigner::operator>>(QDataStream& stream, ItemLibraryInfo& image);
public:
    ItemLibraryInfo();
    ~ItemLibraryInfo();

    QString name() const;
    QString typeName() const;
    QIcon icon() const;
    int majorVersion() const;
    int minorVersion() const;
    QString category() const;
    QIcon dragIcon() const;
    QString qml() const;

    ItemLibraryInfo(const ItemLibraryInfo &other);
    ItemLibraryInfo& operator=(const ItemLibraryInfo &other);

    typedef QmlDesigner::PropertyContainer Property;

    QList<Property> properties() const;

    void setTypeName(const QString &typeName);
    void setMajorVersion(int majorNumber);
    void setMinorVersion(int minorNumber);
    void setName(const QString &name);
    void setIcon(const QIcon &icon);
    void addProperty(const Property &p);
    void addProperty(QString &name, QString &type, QString &value);
    void setDragIcon(const QIcon &icon);
    void setCategory(const QString &category);
    void setQml(const QString &qml);
private:
    QExplicitlySharedDataPointer<Internal::ItemLibraryInfoData> m_data;
};

}

Q_DECLARE_METATYPE(QmlDesigner::ItemLibraryInfo)

#endif // ItemLibraryINFO_H
