/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "portlist.h"

#include <QPair>
#include <QString>
#include <QStringList>

#include <cctype>

namespace Utils {
namespace Internal {
namespace {

typedef QPair<Port, Port> Range;

class PortsSpecParser
{
    struct ParseException {
        ParseException(const char *error) : error(error) {}
        const char * const error;
    };

public:
    PortsSpecParser(const QString &portsSpec)
        : m_pos(0), m_portsSpec(portsSpec) { }

    /*
     * Grammar: Spec -> [ ElemList ]
     *          ElemList -> Elem [ ',' ElemList ]
     *          Elem -> Port [ '-' Port ]
     */
    PortList parse()
    {
        try {
            if (!atEnd())
                parseElemList();
        } catch (const ParseException &e) {
            qWarning("Malformed ports specification: %s", e.error);
        }
        return m_portList;
    }

private:
    void parseElemList()
    {
        if (atEnd())
            throw ParseException("Element list empty.");
        parseElem();
        if (atEnd())
            return;
        if (nextChar() != ',') {
            throw ParseException("Element followed by something else "
                "than a comma.");
        }
        ++m_pos;
        parseElemList();
    }

    void parseElem()
    {
        const Port startPort = parsePort();
        if (atEnd() || nextChar() != '-') {
            m_portList.addPort(startPort);
            return;
        }
        ++m_pos;
        const Port endPort = parsePort();
        if (endPort < startPort)
            throw ParseException("Invalid range (end < start).");
        m_portList.addRange(startPort, endPort);
    }

    Port parsePort()
    {
        if (atEnd())
            throw ParseException("Empty port string.");
        int port = 0;
        do {
            const char next = nextChar();
            if (!std::isdigit(next))
                break;
            port = 10*port + next - '0';
            ++m_pos;
        } while (!atEnd());
        if (port == 0 || port >= 2 << 16)
            throw ParseException("Invalid port value.");
        return Port(port);
    }

    bool atEnd() const { return m_pos == m_portsSpec.length(); }
    char nextChar() const { return m_portsSpec.at(m_pos).toLatin1(); }

    PortList m_portList;
    int m_pos;
    const QString &m_portsSpec;
};

} // anonymous namespace

class PortListPrivate
{
public:
    QList<Range> ranges;
};

} // namespace Internal

PortList::PortList() : d(new Internal::PortListPrivate)
{
}

PortList::PortList(const PortList &other) : d(new Internal::PortListPrivate(*other.d))
{
}

PortList::~PortList()
{
    delete d;
}

PortList &PortList::operator=(const PortList &other)
{
    *d = *other.d;
    return *this;
}

PortList PortList::fromString(const QString &portsSpec)
{
    return Internal::PortsSpecParser(portsSpec).parse();
}

void PortList::addPort(Port port) { addRange(port, port); }

void PortList::addRange(Port startPort, Port endPort)
{
    d->ranges << Internal::Range(startPort, endPort);
}

bool PortList::hasMore() const { return !d->ranges.isEmpty(); }

bool PortList::contains(Port port) const
{
    foreach (const Internal::Range &r, d->ranges) {
        if (port >= r.first && port <= r.second)
            return true;
    }
    return false;
}

int PortList::count() const
{
    int n = 0;
    foreach (const Internal::Range &r, d->ranges)
        n += r.second.number() - r.first.number() + 1;
    return n;
}

Port PortList::getNext()
{
    Q_ASSERT(!d->ranges.isEmpty());

    Internal::Range &firstRange = d->ranges.first();
    const Port next = firstRange.first;
    firstRange.first = Port(firstRange.first.number() + 1);
    if (firstRange.first > firstRange.second)
        d->ranges.removeFirst();
    return next;
}

QString PortList::toString() const
{
    QString stringRep;
    foreach (const Internal::Range &range, d->ranges) {
        stringRep += QString::number(range.first.number());
        if (range.second != range.first)
            stringRep += QLatin1Char('-') + QString::number(range.second.number());
        stringRep += QLatin1Char(',');
    }
    if (!stringRep.isEmpty())
        stringRep.remove(stringRep.length() - 1, 1); // Trailing comma.
    return stringRep;
}

QString PortList::regularExpression()
{
    const QLatin1String portExpr("(\\d)+");
    const QString listElemExpr = QString::fromLatin1("%1(-%1)?").arg(portExpr);
    return QString::fromLatin1("((%1)(,%1)*)?").arg(listElemExpr);
}

} // namespace Utils
