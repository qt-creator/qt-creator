/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#ifndef PORTLIST_H
#define PORTLIST_H

#include "remotelinux_export.h"

QT_FORWARD_DECLARE_CLASS(QString)

namespace RemoteLinux {
namespace Internal {
class PortListPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT PortList
{
public:
    PortList();
    PortList(const PortList &other);
    PortList &operator=(const PortList &other);

    void addPort(int port);
    void addRange(int startPort, int endPort);
    bool hasMore() const;
    bool contains(int port) const;
    int count() const;
    int getNext();
    QString toString() const;

    static PortList fromString(const QString &portsSpec);
    static QString regularExpression();

private:
    Internal::PortListPrivate * const d;
};

} // namespace RemoteLinux

#endif // PORTLIST_H
