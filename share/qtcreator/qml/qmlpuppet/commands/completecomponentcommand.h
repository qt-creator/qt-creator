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

#ifndef COMPLETECOMPONENT_H
#define COMPLETECOMPONENT_H

#include <QMetaType>
#include <QVector>
#include <QDataStream>

namespace QmlDesigner {

class CompleteComponentCommand
{
    friend QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);
    friend QDebug operator <<(QDebug debug, const CompleteComponentCommand &command);

public:
    CompleteComponentCommand();
    explicit CompleteComponentCommand(const QVector<qint32> &container);

    QVector<qint32> instances() const;

private:
    QVector<qint32> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const CompleteComponentCommand &command);
QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);

QDebug operator <<(QDebug debug, const CompleteComponentCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CompleteComponentCommand)

#endif // COMPLETECOMPONENT_H
