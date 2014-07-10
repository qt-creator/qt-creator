/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
