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

#include "callgrindparsedata.h"

#include "callgrindfunction.h"
#include "callgrindcycledetection.h"
#include "callgrindfunctioncycle.h"

#include <utils/qtcassert.h>

#include <QStringList>
#include <QVector>
#include <QHash>
#include <QCoreApplication>

namespace Valgrind {
namespace Callgrind {

//BEGIN ParseData::Private

class ParseData::Private {
    Q_DECLARE_TR_FUNCTIONS(Valgrind::Callgrind::ParseData)
public:
    Private(ParseData *q)
    : m_lineNumberPositionIndex(-1)
    , m_pid(0)
    , m_part(0)
    , m_version(0)
    , m_cycleCacheValid(false)
    , m_q(q)
    {
    }

    ~Private();

    QStringList m_events;
    QStringList m_positions;
    int m_lineNumberPositionIndex;
    QVector<quint64> m_totalCosts;
    QVector<const Function *> m_functions;
    QString m_command;
    quint64 m_pid;
    uint m_part;
    QStringList m_descriptions;
    int m_version;
    QString m_creator;

    QHash<qint64, QHash<qint64, QVector<Function *> > > functionLookup;

    typedef QHash<qint64, QString> NameLookupTable;
    QString stringForCompression(const NameLookupTable &lookup, qint64 id);
    void addCompressedString(NameLookupTable &lookup, const QString &string, qint64 &id);

    NameLookupTable m_objectCompression;
    NameLookupTable m_fileCompression;
    NameLookupTable m_functionCompression;

    void cycleDetection();
    void cleanupFunctionCycles();
    bool m_cycleCacheValid;
    QVector<const Function *> m_cycleCache;

    ParseData *m_q;
};

ParseData::Private::~Private()
{
    cleanupFunctionCycles();
    qDeleteAll(m_functions);
}

void ParseData::Private::cleanupFunctionCycles()
{
    m_cycleCacheValid = false;
    foreach (const Function *func, m_cycleCache) {
        if (dynamic_cast<const FunctionCycle *>(func))
            delete func;
    }
    m_cycleCache.clear();
}

QString ParseData::Private::stringForCompression(const NameLookupTable &lookup, qint64 id)
{
    if (id == -1) {
        return QString();
    } else {
        QTC_ASSERT(lookup.contains(id), return QString());
        return lookup.value(id);
    }
}

void ParseData::Private::addCompressedString(NameLookupTable &lookup, const QString &string,
                                             qint64 &id)
{
    QTC_ASSERT(!string.isEmpty(), return);

    if (id == -1) {
        // for uncompressed files, use a hash of the string
        id = qHash(string);

        if (lookup.contains(id)) {
            QTC_ASSERT(lookup.value(id) == string, return);
            return;
        }
    }

    QTC_ASSERT(!lookup.contains(id), return);
    lookup.insert(id, string);
}

void ParseData::Private::cycleDetection()
{
    if (m_cycleCacheValid)
        return;
    cleanupFunctionCycles();
    Internal::CycleDetection algorithm(m_q);
    m_cycleCache = algorithm.run(m_functions);
    m_cycleCacheValid = true;
}

//BEGIN ParseData

ParseData::ParseData()
: d(new Private(this))
{

}

ParseData::~ParseData()
{
    delete d;
}

QString ParseData::prettyStringForEvent(const QString &event)
{
    /*
        From Callgrind documentation, see: http://valgrind.org/docs/manual/cg-manual.html#cg-manual.overview

        I cache reads (Ir, which equals the number of instructions executed),
        I1 cache read misses (I1mr) and LL cache instruction read misses (ILmr).
        D cache reads (Dr, which equals the number of memory reads),
        D1 cache read misses (D1mr), and LL cache data read misses (DLmr).
        D cache writes (Dw, which equals the number of memory writes),
        D1 cache write misses (D1mw), and LL cache data write misses (DLmw).
        Conditional branches executed (Bc) and conditional branches mispredicted (Bcm).
        Indirect branches executed (Bi) and indirect branches mispredicted (Bim)
    */

    QTC_ASSERT(event.size() >= 2, return event); // should not happen

    const bool isMiss = event.contains(QLatin1Char('m')); // else hit
    const bool isRead = event.contains(QLatin1Char('r')); // else write

    QString type;
    if (event.contains(QLatin1Char('L')))
        type = ParseData::Private::tr("Last-level"); // first, "L" overwrites the others
    else if (event.at(0) == QLatin1Char('I'))
        type = ParseData::Private::tr("Instruction");
    else if (event.at(0) == QLatin1Char('D'))
        type = ParseData::Private::tr("Cache");
    else if (event.leftRef(2) == QLatin1String("Bc"))
        type = ParseData::Private::tr("Conditional branches");
    else if (event.leftRef(2) == QLatin1String("Bi"))
        type = ParseData::Private::tr("Indirect branches");

    QStringList prettyString;
    prettyString << type;

    if (event.at(1).isNumber())
        prettyString << ParseData::Private::tr("level %1").arg(event.at(1));
    prettyString << (isRead ? ParseData::Private::tr("read") : ParseData::Private::tr("write"));

    if (event.at(0) == QLatin1Char('B'))
        prettyString << (isMiss ? ParseData::Private::tr("mispredicted") : ParseData::Private::tr("executed"));
    else
        prettyString << (isMiss ? ParseData::Private::tr("miss") : ParseData::Private::tr("access"));

    // add original abbreviation
    prettyString << QLatin1Char('(') + event + QLatin1Char(')');
    return prettyString.join(QString(QLatin1Char(' ')));
}

QStringList ParseData::events() const
{
    return d->m_events;
}

void ParseData::setEvents(const QStringList &events)
{
    d->m_events = events;
    d->m_totalCosts.fill(0, d->m_events.size());
}

QString ParseData::prettyStringForPosition(const QString &position)
{
    if (position == QLatin1String("line"))
        return ParseData::Private::tr("Line:"); // as in: "line number"
    else if (position == QLatin1String("instr"))
        return ParseData::Private::tr("Instruction"); // as in: "instruction address"
    return ParseData::Private::tr("Position:"); // never reached, in theory
}

QStringList ParseData::positions() const
{
    return d->m_positions;
}

int ParseData::lineNumberPositionIndex() const
{
    return d->m_lineNumberPositionIndex;
}

void ParseData::setPositions(const QStringList &positions)
{
    d->m_positions = positions;
    d->m_lineNumberPositionIndex = -1;
    for (int i = 0; i < positions.size(); ++i) {
        if (positions.at(i) == QLatin1String("line")) {
            d->m_lineNumberPositionIndex = i;
            break;
        }
    }
}

quint64 ParseData::totalCost(uint event) const
{
    return d->m_totalCosts.at(event);
}

void ParseData::setTotalCost(uint event, quint64 cost)
{
    d->m_totalCosts[event] = cost;
}

QVector<const Function *> ParseData::functions(bool detectCycles) const
{
    if (detectCycles) {
        d->cycleDetection();
        return d->m_cycleCache;
    }
    return d->m_functions;
}

void ParseData::addFunction(const Function *function)
{
    d->m_cycleCacheValid = false;
    d->m_functions.append(function);
}

QString ParseData::command() const
{
    return d->m_command;
}

void ParseData::setCommand(const QString &command)
{
    d->m_command = command;
}

quint64 ParseData::pid() const
{
    return d->m_pid;
}

void ParseData::setPid(quint64 pid)
{
    d->m_pid = pid;
}

uint ParseData::part() const
{
    return d->m_part;
}

void ParseData::setPart(uint part) const
{
    d->m_part = part;
}

QStringList ParseData::descriptions() const
{
    return d->m_descriptions;
}

void ParseData::addDescription(const QString &description)
{
    d->m_descriptions.append(description);
}

void ParseData::setDescriptions(const QStringList &descriptions)
{
    d->m_descriptions = descriptions;
}

int ParseData::version() const
{
    return d->m_version;
}

void ParseData::setVersion(int version)
{
    d->m_version = version;
}

QString ParseData::creator() const
{
    return d->m_creator;
}

void ParseData::setCreator(const QString &creator)
{
    d->m_creator = creator;
}

QString ParseData::stringForObjectCompression(qint64 id) const
{
    return d->stringForCompression(d->m_objectCompression, id);
}

void ParseData::addCompressedObject(const QString &object, qint64 &id)
{
    d->addCompressedString(d->m_objectCompression, object, id);
}

QString ParseData::stringForFileCompression(qint64 id) const
{
    return d->stringForCompression(d->m_fileCompression, id);
}

void ParseData::addCompressedFile(const QString &file, qint64 &id)
{
    d->addCompressedString(d->m_fileCompression, file, id);
}

QString ParseData::stringForFunctionCompression(qint64 id) const
{
    return d->stringForCompression(d->m_functionCompression, id);
}

void ParseData::addCompressedFunction(const QString &function, qint64 &id)
{
    d->addCompressedString(d->m_functionCompression, function, id);
}

} // namespace Callgrind
} // namespace Valgrind
