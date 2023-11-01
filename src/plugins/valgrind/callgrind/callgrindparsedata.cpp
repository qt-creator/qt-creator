// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindparsedata.h"

#include "callgrindcycledetection.h"
#include "callgrindfunction.h"
#include "callgrindfunctioncycle.h"
#include "../valgrindtr.h"

#include <utils/qtcassert.h>

#include <QHash>
#include <QStringList>

namespace Valgrind::Callgrind {

class ParseData::Private
{
public:
    Private(ParseData *q, const QString &fileName)
        : m_fileName(fileName)
        , m_q(q)
    {
    }

    ~Private();

    QString m_fileName;
    QStringList m_events;
    QStringList m_positions;
    QList<quint64> m_totalCosts;
    QList<const Function *> m_functions;
    QString m_command;
    quint64 m_pid = 0;
    int m_lineNumberPositionIndex = -1;
    uint m_part = 0;
    int m_version = 0;
    bool m_cycleCacheValid = false;
    QStringList m_descriptions;
    QString m_creator;

    QHash<qint64, QHash<qint64, QList<Function *>>> functionLookup;

    using NameLookupTable = QHash<qint64, QString>;
    QString stringForCompression(const NameLookupTable &lookup, qint64 id);
    void addCompressedString(NameLookupTable &lookup, const QString &string, qint64 &id);

    NameLookupTable m_objectCompression;
    NameLookupTable m_fileCompression;
    NameLookupTable m_functionCompression;

    void cycleDetection();
    void cleanupFunctionCycles();
    QList<const Function *> m_cycleCache;

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
    for (const Function *func : std::as_const(m_cycleCache)) {
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

ParseData::ParseData(const QString &fileName)
    : d(new Private(this, fileName))
{
}

ParseData::~ParseData()
{
    delete d;
}

QString ParseData::fileName() const
{
    return d->m_fileName;
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

    const bool isMiss = event.contains('m'); // else hit
    const bool isRead = event.contains('r'); // else write

    QString type;
    if (event.contains('L'))
        type = Tr::tr("Last-level"); // first, "L" overwrites the others
    else if (event.at(0) == 'I')
        type = Tr::tr("Instruction");
    else if (event.at(0) == 'D')
        type = Tr::tr("Cache");
    else if (event.left(2) == "Bc")
        type = Tr::tr("Conditional branches");
    else if (event.left(2) == "Bi")
        type = Tr::tr("Indirect branches");

    QStringList prettyString;
    prettyString << type;

    if (event.at(1).isNumber())
        prettyString << Tr::tr("level %1").arg(event.at(1));
    prettyString << (isRead ? Tr::tr("read") : Tr::tr("write"));

    if (event.at(0) == 'B')
        prettyString << (isMiss ? Tr::tr("mispredicted") : Tr::tr("executed"));
    else
        prettyString << (isMiss ? Tr::tr("miss") : Tr::tr("access"));

    // add original abbreviation
    prettyString << '(' + event + ')';
    return prettyString.join(' ');
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
    if (position == "line")
        return Tr::tr("Line:"); // as in: "line number"
    if (position == "instr")
        return Tr::tr("Instruction"); // as in: "instruction address"
    return Tr::tr("Position:"); // never reached, in theory
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
        if (positions.at(i) == "line") {
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

QList<const Function *> ParseData::functions(bool detectCycles) const
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

} // namespace Valgrind::Callgrind
