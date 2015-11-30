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

#ifndef INSTANCECONTAINER_H
#define INSTANCECONTAINER_H

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

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
    InstanceContainer(qint32 instanceId, const TypeName &type, int majorNumber, int minorNumber, const QString &componentPath, const QString &nodeSource, NodeSourceType nodeSourceType, NodeMetaType metaType);

    qint32 instanceId() const;
    TypeName type() const;
    int majorNumber() const;
    int minorNumber() const;
    QString componentPath() const;
    QString nodeSource() const;
    NodeSourceType nodeSourceType() const;
    NodeMetaType metaType() const;

private:
    qint32 m_instanceId;
    TypeName m_type;
    qint32 m_majorNumber;
    qint32 m_minorNumber;
    QString m_componentPath;
    QString m_nodeSource;
    qint32 m_nodeSourceType;
    qint32 m_metaType;
};

QDebug operator <<(QDebug debug, const InstanceContainer &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InstanceContainer)
#endif // INSTANCECONTAINER_H
