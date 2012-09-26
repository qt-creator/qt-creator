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

#ifndef REPARENTCONTAINER_H
#define REPARENTCONTAINER_H

#include <qmetatype.h>
#include <QDataStream>

namespace QmlDesigner {

class ReparentContainer
{
    friend QDataStream &operator>>(QDataStream &in, ReparentContainer &container);
public:
    ReparentContainer();
    ReparentContainer(qint32 instanceId,
                      qint32 oldParentInstanceId,
                      const QString &oldParentProperty,
                      qint32 newParentInstanceId,
                      const QString &newParentProperty);

    qint32 instanceId() const;
    qint32 oldParentInstanceId() const;
    QString oldParentProperty() const;
    qint32 newParentInstanceId() const;
    QString newParentProperty() const;

private:
    qint32 m_instanceId;
    qint32 m_oldParentInstanceId;
    QString m_oldParentProperty;
    qint32 m_newParentInstanceId;
    QString m_newParentProperty;
};

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container);
QDataStream &operator>>(QDataStream &in, ReparentContainer &container);

} // namespace QmlDesigner

#endif // REPARENTCONTAINER_H
