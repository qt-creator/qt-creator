/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "childrenchangedcommand.h"

namespace QmlDesigner {

ChildrenChangedCommand::ChildrenChangedCommand()
    : m_parentInstanceId(-1)
{
}

ChildrenChangedCommand::ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &children, const QVector<InformationContainer> &informationVector)
    : m_parentInstanceId(parentInstanceId),
      m_childrenVector(children),
      m_informationVector(informationVector)
{
}

QVector<qint32> ChildrenChangedCommand::childrenInstances() const
{
    return m_childrenVector;
}

qint32 ChildrenChangedCommand::parentInstanceId() const
{
    return m_parentInstanceId;
}

QVector<InformationContainer> ChildrenChangedCommand::informations() const
{
    return m_informationVector;
}

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command)
{
    out << command.parentInstanceId();
    out << command.childrenInstances();
    out << command.informations();
    return out;
}

QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command)
{
    in >> command.m_parentInstanceId;
    in >> command.m_childrenVector;
    in >> command.m_informationVector;

    return in;
}

} // namespace QmlDesigner
