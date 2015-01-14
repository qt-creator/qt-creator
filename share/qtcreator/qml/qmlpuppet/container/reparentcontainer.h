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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
