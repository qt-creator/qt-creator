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

#ifndef INSTANCECONTAINER_H
#define INSTANCECONTAINER_H

#include <qmetatype.h>
#include <QString>

namespace QmlDesigner {

class InstanceContainer;

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container);
QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

class InstanceContainer
{
    friend QDataStream &operator>>(QDataStream &in, InstanceContainer &container);

public:
    enum NodeSourceType {
        NoSource = 0,
        CustomParserSource = 1,
        ComponentSource = 2
    };

    enum NodeMetaType {
        ObjectMetaType,
        ItemMetaType
    };

    InstanceContainer();
    InstanceContainer(qint32 instanceId, const QString &type, int majorNumber, int minorNumber, const QString &componentPath, const QString &nodeSource, NodeSourceType nodeSourceType, NodeMetaType metaType);

    qint32 instanceId() const;
    QString type() const;
    int majorNumber() const;
    int minorNumber() const;
    QString componentPath() const;
    QString nodeSource() const;
    NodeSourceType nodeSourceType() const;
    NodeMetaType metaType() const;

private:
    qint32 m_instanceId;
    QString m_type;
    qint32 m_majorNumber;
    qint32 m_minorNumber;
    QString m_componentPath;
    QString m_nodeSource;
    qint32 m_nodeSourceType;
    qint32 m_metaType;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InstanceContainer)
#endif // INSTANCECONTAINER_H
