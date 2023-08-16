// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callgrindparser.h"

#include "callgrindcostitem.h"
#include "callgrindfunction.h"
#include "callgrindfunctioncall.h"
#include "callgrindparsedata.h"

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QDebug>
#include <QHash>
#include <QStringList>

// #define DEBUG_PARSER

using namespace Utils;

namespace Valgrind::Callgrind {

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


class Parser::Private
{
    Parser *const q;
public:

    explicit Private(Parser *qq)
        : q(qq)
    {
    }

    ~Private()
    {
        delete data;
    }

    void parse(const FilePath &filePath);
    void parseHeader(QIODevice *device);

    using NamePair = QPair<qint64, QString>;
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

    int addressValuesCount = 0;
    int costValuesCount = 0;

    ParseData *data = nullptr;
    Function *currentFunction = nullptr;
    qint64 lastObject = -1;
    qint64 lastFile = -1;
    qint64 currentDifferingFile = -1;

    bool isParsingFunctionCall = false;
    quint64 callsCount = 0;
    struct CallData {
        qint64 calledFunction = -1;
        qint64 calledObject = -1;
        qint64 calledFile = -1;
        FunctionCall *call = nullptr;
    };
    CallData currentCallData;
    QList<quint64> callDestinations;

    // we can only resolve callees after parsing the whole file so save that data here for now
    QList<CallData> pendingCallees;

    // id(s) for the ??? file
    QList<quint64> unknownFiles;

    // functions which call themselves
    QSet<Function *> recursiveFunctions;
};

void Parser::Private::parse(const FilePath &filePath)
{
    // be sure to clean up existing data before re-allocating
    // the callee might not have taken the parse data
    delete data;
    data = nullptr;

    const QString path = filePath.path(); // FIXME: Works only accidentally for docker
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        qWarning() << "Could not open file for parsing:" << filePath.toUserOutput();

    data = new ParseData(path);
    parseHeader(&file);
    while (!file.atEnd()) {
        const QByteArray line = file.readLine();
        // empty lines actually have no meaning - only fn= starts a new function
        if (line.length() > 1)
            dispatchLine(line);
    }

    // build fast lookup of functions by their nameId
    QHash<qint64, QList<const Function *> > functionLookup;
    const QList<const Function *> functions = data->functions();
    for (const Function *function : functions) {
        functionLookup[function->nameId()].append(function);
    }

    // functions that need to accumulate their calees
    QSet<Function *> pendingFunctions;
    for (const CallData &callData : std::as_const(pendingCallees)) {
        Function *calledFunction = nullptr;
        QTC_ASSERT(callData.call, continue);
        QTC_ASSERT(callData.call->caller(), continue);
        const QList<const Function *> functions = functionLookup.value(callData.calledFunction);
        for (const Function *function : functions) {
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
            for (const Function *function, functionLookup.value(callData.calledFunction)) {
                qDebug() << "available function file:" << function->fileId() << function->file() << "object:" << function->objectId() << function->object();
            }
        }
#endif
        QTC_ASSERT(calledFunction, continue);
        callData.call->setCallee(calledFunction);
        calledFunction->addIncomingCall(callData.call);

        auto caller = const_cast<Function *>(callData.call->caller());
        caller->addOutgoingCall(callData.call);
        pendingFunctions.insert(caller);
    }

    pendingCallees.clear();
    // lookup done

    // now accumulate callees
    for (Function *func : std::as_const(pendingFunctions))
        func->finalize();

    emit q->parserDataReady();
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

        // last character will be ignored anyhow, but we might have CRLF; if so cut the last one
        if (line.endsWith("\r\n"))
            line.chop(1);

        // now that we're done checking if we're done (heh) with the header, parse the address
        // and cost column descriptions. speed is unimportant here.
        if (line.startsWith('#')) {
            continue;
        } else if (line.startsWith("positions: ")) {
            QString values = getValue(line, 11);
            data->setPositions(values.split(' ', Qt::SkipEmptyParts));
            addressValuesCount = data->positions().count();
        } else if (line.startsWith("events: ")) {
            QString values = getValue(line, 8);
            data->setEvents(values.split(' ', Qt::SkipEmptyParts));
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
            const QStringList valueList = values.split(' ', Qt::SkipEmptyParts);
            for (const QString &value : valueList)
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
            return {qint64(-1), {}}; // error
    }

    skipSpace(&current, end);
    return {nameShorthand, QString::fromUtf8(QByteArray(current, end - current))};
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
    int lineEnding = line.endsWith("\r\n") ? 2 : 1;
    const char *const begin = line.constData();
    const char *const end = begin + line.length() - lineEnding; // we're not interested in the '\n'
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

    QTC_ASSERT(currentDifferingFile == -1 || currentDifferingFile != currentFunction->fileId(), return);
    auto costItem = new CostItem(data);
    costItem->setDifferingFile(currentDifferingFile);
    FunctionCall *call = nullptr;
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
            if (unknownFiles.contains(quint64(currentCallData.calledFile)))
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

    const CostItem *lastCostItem = nullptr;
    if (!currentFunction->costItems().isEmpty())
        lastCostItem = currentFunction->costItems().constLast();

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
        if (name.second == "???")
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
        if (name.second == "???")
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
        if (name.second == "???")
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

void Parser::parse(const Utils::FilePath &filePath)
{
    d->parse(filePath);
}

Parser::Parser()
    : d(new Private(this))
{
}

Parser::~Parser()
{
    delete d;
}

ParseData *Parser::takeData()
{
    ParseData *data = d->data;
    d->data = nullptr;
    return data;
}

} // namespace Valgrind::Callgrind
