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

#include "callgrindparser.h"

#include "callgrindparsedata.h"
#include "callgrindfunctioncall.h"
#include "callgrindcostitem.h"
#include "callgrindfunction.h"

#include <utils/qtcassert.h>

#include <QHash>
#include <QVector>
#include <QStringList>
#include <QDebug>

// #define DEBUG_PARSER

namespace {

static void skipSpace(const char **current, const char *end)
{
    const char *b = *current;
    while (b < end) {
        if (*b == ' ' || *b == '\t')
            b++;
        else
            break;
    }
    *current = b;
}

// set *ok to true if at least one digit was parsed; "garbage" after the number is not considered
// an error.
// *current is moved to one char after the last digit
static qint64 parseDecimal(const char **current, const char *end, bool *ok)
{
    const char *b = *current;
    bool parsedDigit = false;
    qint64 ret = 0;
    while (b < end) {
        const char c = *b;
        if (c >= '0' && c <= '9') {
            b++;
            ret *= 10;
            ret += c - '0';
            parsedDigit = true;
        } else {
            break;
        }
    }

    *ok = parsedDigit;
    *current = b;
    return ret;
}

//like parseDecimal, but for 0xabcd-style hex encoding (without the leading 0x)
static qint64 parseHex(const char **current, const char *end, bool *ok)
{
    const char *b = *current;
    bool parsedDigit = false;
    qint64 ret = 0;
    while (b < end) {
        char c = *b;
        if (c >= '0' && c <= '9')
            c &= 0x0f;
        else if (c >= 'a' && c <= 'f')
            c = c - 'a' + 10;
        else
            break;

        b++;
        ret <<= 4;
        ret += c;
        parsedDigit = true;
    }

    *ok = parsedDigit;
    *current = b;
    return ret;
}

static quint64 parseAddr(const char **current, const char *end, bool *ok)
{
    if (**current == '0' && *(*current + 1) == 'x') {
        *current += 2;
        return parseHex(current, end, ok);
    } else {
        return parseDecimal(current, end, ok);
    }
}

// this function expects that *current already points one past the opening parenthesis
static int parseNameShorthand(const char **current, const char *end)
{
    bool ok;
    int ret = parseDecimal(current, end, &ok);
    if (ok) {
        if (**current == ')') {
            (*current)++;
            return ret;
        }
    }
    return -1; // invalid
}

}

namespace Valgrind {
namespace Callgrind {

class Parser::Private
{
    Parser *const q;
public:

    explicit Private(Parser *qq)
        : q(qq),
          addressValuesCount(0),
          costValuesCount(0),
          data(0),
          currentFunction(0),
          lastObject(-1),
          lastFile(-1),
          currentDifferingFile(-1),
          isParsingFunctionCall(false),
          callsCount(0)
    {
    }

    ~Private()
    {
        delete data;
    }

    void parse(QIODevice *device);
    void parseHeader(QIODevice *device);

    typedef QPair<qint64, QString> NamePair;
    NamePair parseName(const char *begin, const char *end);

    void dispatchLine(const QByteArray &line);
    void parseCostItem(const char *begin, const char *end);
    void parseFunction(const char *begin, const char *end);
    void parseSourceFile(const char *begin, const char *end);
    void parseDifferingSourceFile(const char *begin, const char *end);
    void parseObjectFile(const char *begin, const char *end);
    void parseCalls(const char *begin, const char *end);
    void parseCalledFunction(const char *begin, const char *end);
    void parseCalledSourceFile(const char *begin, const char *end);
    void parseCalledObjectFile(const char *begin, const char *end);

    int addressValuesCount;
    int costValuesCount;

    ParseData *data;
    Function *currentFunction;
    qint64 lastObject;
    qint64 lastFile;
    qint64 currentDifferingFile;

    bool isParsingFunctionCall;
    quint64 callsCount;
    struct CallData {
        CallData()
        : calledFunction(-1)
        , calledObject(-1)
        , calledFile(-1)
        , call(0)
        {
        }

        qint64 calledFunction;
        qint64 calledObject;
        qint64 calledFile;
        FunctionCall *call;
    };
    CallData currentCallData;
    QVector<quint64> callDestinations;

    // we can only resolve callees after parsing the whole file so save that data here for now
    QVector<CallData> pendingCallees;

    // id(s) for the ??? file
    QVector<quint64> unknownFiles;

    // functions which call themselves
    QSet<Function *> recursiveFunctions;
};

void Parser::Private::parse(QIODevice *device)
{
    // be sure to clean up existing data before re-allocating
    // the callee might not have taken the parse data
    delete data;
    data = 0;

    data = new ParseData;
    parseHeader(device);
    while (!device->atEnd()) {
        QByteArray line = device->readLine();
        // empty lines actually have no meaning - only fn= starts a new function
        if (line.length() > 1)
            dispatchLine(line);
    }

    // build fast lookup of functions by their nameId
    QHash<qint64, QList<const Function *> > functionLookup;
    foreach (const Function *function, data->functions()) {
        functionLookup[function->nameId()].append(function);
    }

    // functions that need to accumulate their calees
    QSet<Function *> pendingFunctions;
    foreach (const CallData &callData, pendingCallees) {
        Function *calledFunction = 0;
        QTC_ASSERT(callData.call, continue);
        QTC_ASSERT(callData.call->caller(), continue);
        foreach (const Function *function, functionLookup.value(callData.calledFunction)) {
            QTC_ASSERT(function->nameId() == callData.calledFunction, continue);
            if (function->objectId() == callData.calledObject
                && function->fileId() == callData.calledFile)
            {
                calledFunction = const_cast<Function *>(function);
                break;
            }
        }
#ifdef DEBUG_PARSER
        if (!calledFunction) {
            qDebug() << unknownFiles;
            qDebug() << "could not find called function:" << data->stringForFunctionCompression(callData.calledFunction) << callData.calledFunction;
            qDebug() << "caller is:" << callData.call->caller()->name() << callData.call->caller()->nameId();
            qDebug() << "called file:" << callData.calledFile << "object:" << callData.calledObject;
            qDebug() << data->stringForFileCompression(callData.calledFile) << data->stringForObjectCompression(callData.calledObject);
            foreach (const Function *function, functionLookup.value(callData.calledFunction)) {
                qDebug() << "available function file:" << function->fileId() << function->file() << "object:" << function->objectId() << function->object();
            }
        }
#endif
        QTC_ASSERT(calledFunction, continue);
        callData.call->setCallee(calledFunction);
        calledFunction->addIncomingCall(callData.call);

        Function *caller = const_cast<Function *>(callData.call->caller());
        caller->addOutgoingCall(callData.call);
        pendingFunctions.insert(caller);
    }

    pendingCallees.clear();
    // lookup done

    // now accumulate callees
    foreach (Function *func, pendingFunctions)
        func->finalize();

    q->parserDataReady(); // emit
}

inline QString getValue(const QByteArray &line, const int prefixLength)
{
    // we are not interested in the trailing newline
    // TODO: \r\n ?
    return QString::fromLatin1(line.mid(prefixLength, line.length() - 1 - prefixLength).constData());
}

void Parser::Private::parseHeader(QIODevice *device)
{
    QTC_ASSERT(device->isOpen(), return);
    QTC_ASSERT(device->isReadable(), return);

    // parse expected headers until we hit the first non-empty line
    while (!device->atEnd()) {
        QByteArray line = device->readLine();

        // now that we're done checking if we're done (heh) with the header, parse the address
        // and cost column descriptions. speed is unimportant here.
        if (line.startsWith("positions: ")) {
            QString values = getValue(line, 11);
            data->setPositions(values.split(QLatin1Char(' '), QString::SkipEmptyParts));
            addressValuesCount = data->positions().count();
        } else if (line.startsWith("events: ")) {
            QString values = getValue(line, 8);
            data->setEvents(values.split(QLatin1Char(' '), QString::SkipEmptyParts));
            costValuesCount = data->events().count();
        } else if (line.startsWith("version: ")) {
            QString value = getValue(line, 9);
            data->setVersion(value.toInt());
        } else if (line.startsWith("creator: ")) {
            QString value = getValue(line, 9);
            data->setCreator(value);
        } else if (line.startsWith("pid: ")) {
            QString value = getValue(line, 5);
            data->setPid(value.toULongLong());
        } else if (line.startsWith("cmd: ")) {
            QString value = getValue(line, 5);
            data->setCommand(value);
        } else if (line.startsWith("part: ")) {
            QString value = getValue(line, 6);
            data->setPart(value.toUInt());
        } else if (line.startsWith("desc: ")) {
            QString value = getValue(line, 6);
            data->addDescription(value);
        } else if (line.startsWith("summary: ")) {
            QString values = getValue(line, 9);
            uint i = 0;
            foreach (const QString &value, values.split(QLatin1Char(' '), QString::SkipEmptyParts))
                data->setTotalCost(i++, value.toULongLong());
        } else if (!line.trimmed().isEmpty()) {
            // handle line and exit parseHeader
            dispatchLine(line);
            return;
        }
    }
}

Parser::Private::NamePair Parser::Private::parseName(const char *begin, const char *end)
{
    const char *current = begin;
    qint64 nameShorthand = -1;
    if (*current == '(') {
        current++;
        if ((nameShorthand = parseNameShorthand(&current, end)) == -1)
            return qMakePair(qint64(-1), QString()); // error
    }

    skipSpace(&current, end);
    return qMakePair(nameShorthand, QString::fromUtf8(QByteArray(current, end - current)));
}

/*
 * fl means source file
 * ob means object file
 * fn means function
 * fe, fi means different source file
 * cfi or cfl means called source file
 * cob means called object file
 * cfn means called function
 */

void Parser::Private::dispatchLine(const QByteArray &line)
{
    const char *const begin = line.constData();
    const char *const end = begin + line.length() - 1; // we're not interested in the '\n'
    const char *current = begin;

    // shortest possible line is "1 1" - a cost item line
    QTC_ASSERT(end - begin >= 3, return);

    const char first = *begin;

    if ((first >= '0' && first <= '9') || first == '+' || first == '*' || first =='-') {
        parseCostItem(begin, end);
        if (isParsingFunctionCall)
            isParsingFunctionCall = false;
        return;
    }

    QTC_ASSERT(!isParsingFunctionCall, return);

    current++;
    const char second = *current++;
    const char third = *current++;
    // current now points to the fourth char...

    if (first == 'c') {
        //  information about a callee
        const char fourth = *current++;
        // current now points to the fifth char...

        switch (second) {
            // comments show the shortest possible line for every case
        case 'a':
        {
            // "calls=1 1", length 9
            QTC_ASSERT(end - begin >= 9, return);
            const char fifth = *current++;
            const char sixth = *current++;
            if (third == 'l' && fourth == 'l' && fifth == 's' && sixth == '=')
                parseCalls(current, end);
            break;
        }
        case 'f':
            QTC_ASSERT(end - begin >= 5, return);
            // "cfi=a" / "cfl=a", length 5
            // "cfn=a", length 5
            if (fourth == '=') {
                if (third == 'i' || third == 'l')
                    parseCalledSourceFile(current, end);
                else if (third == 'n')
                    parseCalledFunction(current, end);
            }
            break;
        case 'o':
            QTC_ASSERT(end - begin >= 5, return);
            // "cob=a", length 5
            if (third == 'b' && fourth == '=')
                parseCalledObjectFile(current, end);
            break;
        default:
            break;
        }

    } else {
        // information about this function
        // shortest possible line is always four chars here, of the type "fl=a"
        QTC_ASSERT(end - begin >= 4, return);
        if (third == '=') {
            if (first == 'f') {
                if (second == 'l')
                    parseSourceFile(current, end);
                else if (second == 'n')
                    parseFunction(current, end);
                else if (second == 'i' || second == 'e')
                    parseDifferingSourceFile(current, end);
            } else if (first == 'o' && second == 'b') {
                parseObjectFile(current, end);
            }
        }
    }
}

void Parser::Private::parseCostItem(const char *begin, const char *end)
{
    QTC_ASSERT(currentFunction, return);

    bool ok;
    const char *current = begin;

    CostItem *costItem = new CostItem(data);
    QTC_ASSERT(currentDifferingFile == -1 || currentDifferingFile != currentFunction->fileId(), return);
    costItem->setDifferingFile(currentDifferingFile);
    FunctionCall *call = 0;
    if (isParsingFunctionCall) {
        call = new FunctionCall;
        call->setCaller(currentFunction);

        currentCallData.call = call;
        costItem->setCall(call);
        call->setCalls(callsCount);
        callsCount = 0;

        call->setDestinations(callDestinations);
        callDestinations.clear();

        if (currentCallData.calledFile == -1) {
            currentCallData.calledFile = currentDifferingFile != -1 ? currentDifferingFile : lastFile;
            //HACK: workaround issue where sometimes fi=??? lines are prepended to function calls
            if (unknownFiles.contains(currentCallData.calledFile))
                currentCallData.calledFile = lastFile;
        }
        if (currentCallData.calledObject == -1)
            currentCallData.calledObject = lastObject;


        if (currentCallData.calledFunction == currentFunction->nameId() &&
            currentCallData.calledFile == currentFunction->fileId() &&
            currentCallData.calledObject == currentFunction->objectId() )
        {
            // recursive call,
            recursiveFunctions << currentFunction;
        }

        pendingCallees.append(currentCallData);
        currentCallData = CallData();
    }

    const CostItem *lastCostItem = 0;
    if (!currentFunction->costItems().isEmpty())
        lastCostItem = currentFunction->costItems().last();

    // parse positions ("where")
    for (int i = 0; i < addressValuesCount; ++i) {
        char c  = *current;
        // TODO overflow checks
        quint64 position = 0;
        if (c == '*') {
            // leave the old value unchanged
            current++;
            QTC_ASSERT(lastCostItem, continue);
            position = lastCostItem->position(i);
        } else {
            if (c == '+' || c == '-')
                current++;

            quint64 addr = parseAddr(&current, end, &ok);

            if (!ok)
                break; /// TODO: error reporting

            if (c == '+') {
                QTC_ASSERT(lastCostItem, continue);
                position = lastCostItem->position(i) + addr;
            } else if (c == '-') {
                QTC_ASSERT(lastCostItem, continue);
                position = lastCostItem->position(i) - addr;
            } else
                position = addr;
        }
        costItem->setPosition(i, position);
        skipSpace(&current, end);
    }

    // parse events ("what")
    for (int i = 0; i < costValuesCount; ++i) {
        quint64 parsedCost = parseDecimal(&current, end, &ok);
        if (!ok)
            break; /// TODO: error reporting
        costItem->setCost(i, parsedCost);
        skipSpace(&current, end);
    }

    if (call)
        call->setCosts(costItem->costs());

    currentFunction->addCostItem(costItem);
}

void Parser::Private::parseSourceFile(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);

    if (!name.second.isEmpty()) {
        data->addCompressedFile(name.second, name.first);
        if (name.second == QLatin1String("???"))
            unknownFiles << name.first;
    }

    lastFile = name.first;
    currentDifferingFile = -1;
}

void Parser::Private::parseFunction(const char *begin, const char *end)
{
    currentFunction = new Function(data);
    currentFunction->setFile(lastFile);
    currentFunction->setObject(lastObject);

    data->addFunction(currentFunction);

    NamePair name = parseName(begin, end);

    if (!name.second.isEmpty())
        data->addCompressedFunction(name.second, name.first);

    currentFunction->setName(name.first);
}

void Parser::Private::parseDifferingSourceFile(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);

    if (!name.second.isEmpty()) {
        data->addCompressedFile(name.second, name.first);
        if (name.second == QLatin1String("???"))
            unknownFiles << name.first;
    }

    if (name.first == currentFunction->fileId())
        currentDifferingFile = -1;
    else
        currentDifferingFile = name.first;
}

void Parser::Private::parseObjectFile(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);
    if (!name.second.isEmpty())
        data->addCompressedObject(name.second, name.first);

    lastObject = name.first;
}

void Parser::Private::parseCalls(const char *begin, const char *end)
{
    const char *current = begin;
    bool ok;
    callsCount = parseDecimal(&current, end, &ok);
    skipSpace(&current, end);

    callDestinations.fill(0, addressValuesCount);
    for (int i = 0; i < addressValuesCount; ++i) {
        callDestinations[i] = parseAddr(&current, end, &ok);
        if (!ok)
            break; // TODO error handling?
        skipSpace(&current, end);
    }

    isParsingFunctionCall = true;
}

void Parser::Private::parseCalledFunction(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);
    if (!name.second.isEmpty())
        data->addCompressedFunction(name.second, name.first);

    currentCallData.calledFunction = name.first;
}

void Parser::Private::parseCalledSourceFile(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);
    if (!name.second.isEmpty()) {
        data->addCompressedFile(name.second, name.first);
        if (name.second == QLatin1String("???"))
            unknownFiles << name.first;
    }

    currentCallData.calledFile = name.first;
}

void Parser::Private::parseCalledObjectFile(const char *begin, const char *end)
{
    NamePair name = parseName(begin, end);
    if (!name.second.isEmpty())
        data->addCompressedObject(name.second, name.first);

    currentCallData.calledObject = name.first;
}

//BEGIN Parser

void Parser::parse(QIODevice *device)
{
    d->parse(device);
}

Parser::Parser(QObject *parent)
    : QObject(parent),
      d(new Private(this))
{
}

Parser::~Parser()
{
    delete d;
}

ParseData *Parser::takeData()
{
    ParseData *data = d->data;
    d->data = 0;
    return data;
}

} //Callgrind
} //Valgrind
