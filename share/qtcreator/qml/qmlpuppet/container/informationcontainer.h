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

#ifndef INFORMATIONCONTAINER_H
#define INFORMATIONCONTAINER_H

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

#include "commondefines.h"

namespace QmlDesigner {

class InformationContainer
{
    friend QDataStream &operator>>(QDataStream &in, InformationContainer &container);
    friend QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
    friend bool operator ==(const InformationContainer &first, const InformationContainer &second);
    friend bool operator <(const InformationContainer &first, const InformationContainer &second);

public:
    InformationContainer();
    InformationContainer(qint32 instanceId,
                         InformationName name,
                         const QVariant &information,
                         const QVariant &secondInformation = QVariant(),
                         const QVariant &thirdInformation = QVariant());

    qint32 instanceId() const;
    InformationName name() const;
    QVariant information() const;
    QVariant secondInformation() const;
    QVariant thirdInformation() const;

private:
    qint32 m_instanceId;
    qint32 m_name;
    QVariant m_information;
    QVariant m_secondInformation;
    QVariant m_thirdInformation;
};

QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
QDataStream &operator>>(QDataStream &in, InformationContainer &container);

bool operator ==(const InformationContainer &first, const InformationContainer &second);
bool operator <(const InformationContainer &first, const InformationContainer &second);
QDebug operator <<(QDebug debug, const InformationContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationContainer)

#endif // INFORMATIONCONTAINER_H
