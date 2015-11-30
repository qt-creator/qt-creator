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

#ifndef QMLDESIGNER_ENUMERATION_H
#define QMLDESIGNER_ENUMERATION_H

#include <QByteArray>
#include <QDataStream>
#include <QMetaType>
#include <QVariant>

namespace QmlDesigner {

typedef QByteArray EnumerationName;

class Enumeration
{
    friend bool operator ==(const Enumeration &first, const Enumeration &second);
    friend bool operator <(const Enumeration &first, const Enumeration &second);
    friend QDataStream &operator>>(QDataStream &in, Enumeration &enumeration);

public:
    Enumeration();
    Enumeration(const EnumerationName &enumerationName);
    Enumeration(const QString &enumerationName);
    Enumeration(const QString &scope, const QString &name);

    EnumerationName scope() const;
    EnumerationName name() const;
    EnumerationName toEnumerationName() const;
    QString toString() const;
    QString nameToString() const;

private:
    EnumerationName m_enumerationName;
};

QDataStream &operator<<(QDataStream &out, const Enumeration &enumeration);
QDataStream &operator>>(QDataStream &in, Enumeration &enumeration);

bool operator ==(const Enumeration &first, const Enumeration &second);
bool operator <(const Enumeration &first, const Enumeration &second);

QDebug operator <<(QDebug debug, const Enumeration &enumeration);


} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Enumeration)

#endif // QMLDESIGNER_ENUMERATION_H
