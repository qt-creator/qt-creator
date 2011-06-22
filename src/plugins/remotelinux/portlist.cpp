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
#include "portlist.h"

#include <cctype>

namespace RemoteLinux {
namespace {

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
        } catch (ParseException &e) {
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
        const int startPort = parsePort();
        if (atEnd() || nextChar() != '-') {
            m_portList.addPort(startPort);
            return;
        }
        ++m_pos;
        const int endPort = parsePort();
        if (endPort < startPort)
            throw ParseException("Invalid range (end < start).");
        m_portList.addRange(startPort, endPort);
    }

    int parsePort()
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
        return port;
    }

    bool atEnd() const { return m_pos == m_portsSpec.length(); }
    char nextChar() const { return m_portsSpec.at(m_pos).toAscii(); }

    PortList m_portList;
    int m_pos;
    const QString &m_portsSpec;
};

} // anonymous namespace


PortList PortList::fromString(const QString &portsSpec)
{
    return PortsSpecParser(portsSpec).parse();
}

void PortList::addPort(int port) { addRange(port, port); }

void PortList::addRange(int startPort, int endPort)
{
    m_ranges << Range(startPort, endPort);
}

bool PortList::hasMore() const { return !m_ranges.isEmpty(); }

int PortList::count() const
{
    int n = 0;
    foreach (const Range &r, m_ranges)
        n += r.second - r.first + 1;
    return n;
}

int PortList::getNext()
{
    Q_ASSERT(!m_ranges.isEmpty());
    Range &firstRange = m_ranges.first();
    const int next = firstRange.first++;
    if (firstRange.first > firstRange.second)
        m_ranges.removeFirst();
    return next;
}

QString PortList::toString() const
{
    QString stringRep;
    foreach (const Range &range, m_ranges) {
        stringRep += QString::number(range.first);
        if (range.second != range.first)
            stringRep += QLatin1Char('-') + QString::number(range.second);
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

} // namespace RemoteLinux
