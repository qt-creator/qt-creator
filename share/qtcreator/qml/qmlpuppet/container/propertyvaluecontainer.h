/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROPERTYVALUECONTAINER_H
#define PROPERTYVALUECONTAINER_H

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

namespace QmlDesigner {


class PropertyValueContainer
{
    friend QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container);

public:
    PropertyValueContainer();
    PropertyValueContainer(qint32 instanceId, const QString &name, const QVariant &value, const QString &dynamicTypeName);

    qint32 instanceId() const;
    QString name() const;
    QVariant value() const;
    bool isDynamic() const;
    QString dynamicTypeName() const;

private:
    qint32 m_instanceId;
    QString m_name;
    QVariant m_value;
    QString m_dynamicTypeName;
};

QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyValueContainer)

#endif // PROPERTYVALUECONTAINER_H
