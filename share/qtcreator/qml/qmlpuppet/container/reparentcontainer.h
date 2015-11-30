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

#ifndef REPARENTCONTAINER_H
#define REPARENTCONTAINER_H

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class ReparentContainer
{
    friend QDataStream &operator>>(QDataStream &in, ReparentContainer &container);
public:
    ReparentContainer();
    ReparentContainer(qint32 instanceId,
                      qint32 oldParentInstanceId,
                      const PropertyName &oldParentProperty,
                      qint32 newParentInstanceId,
                      const PropertyName &newParentProperty);

    qint32 instanceId() const;
    qint32 oldParentInstanceId() const;
    PropertyName oldParentProperty() const;
    qint32 newParentInstanceId() const;
    PropertyName newParentProperty() const;

private:
    qint32 m_instanceId;
    qint32 m_oldParentInstanceId;
    PropertyName m_oldParentProperty;
    qint32 m_newParentInstanceId;
    PropertyName m_newParentProperty;
};

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container);
QDataStream &operator>>(QDataStream &in, ReparentContainer &container);

QDebug operator <<(QDebug debug, const ReparentContainer &container);

} // namespace QmlDesigner

#endif // REPARENTCONTAINER_H
