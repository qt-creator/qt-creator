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

#ifndef CHILDRENCHANGEDCOMMAND_H
#define CHILDRENCHANGEDCOMMAND_H

#include <QMetaType>
#include <QVector>
#include "informationcontainer.h"

namespace QmlDesigner {

class ChildrenChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);
    friend bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);

public:
    ChildrenChangedCommand();
    explicit ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &childrenInstancesconst, const QVector<InformationContainer> &informationVector);

    QVector<qint32> childrenInstances() const;
    qint32 parentInstanceId() const;
    QVector<InformationContainer> informations() const;

    void sort();

private:
    qint32 m_parentInstanceId;
    QVector<qint32> m_childrenVector;
    QVector<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);

bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);
QDebug operator <<(QDebug debug, const ChildrenChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChildrenChangedCommand)

#endif // CHILDRENCHANGEDCOMMAND_H
