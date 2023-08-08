// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "parser.h"
#include "announcethread.h"
#include "error.h"
#include "frame.h"
#include "stack.h"
#include "status.h"
#include "suppression.h"
#include "../valgrindtr.h"

#include <utils/qtcassert.h>

#include <QAbstractSocket>
#include <QHash>
#include <QIODevice>
#include <QMetaEnum>
#include <QXmlStreamReader>

namespace {

    class ParserException
    {
    public:
        explicit ParserException(const QString &message)
            : m_message(message)
        {}

        ~ParserException() noexcept = default;

        QString message() const { return m_message; }

    private:
        QString m_message;
    };

    struct XWhat
    {
        QString text;
        qint64 leakedblocks = 0;
        qint64 leakedbytes = 0;
        qint64 hthreadid = -1;
    };

    struct XauxWhat
    {
        void clear() { *this = XauxWhat(); }

        QString text;
        QString file;
        QString dir;
        qint64 line = -1;
        qint64 hthreadid = -1;
    };

} // namespace anon

namespace Valgrind::XmlProtocol {

enum class Tool {
    Unknown,
    Memcheck,
    Ptrcheck,
    Helgrind
};

const QHash<QString, Tool> &toolByName()
{
    static const QHash<QString, Tool> theHash {
        {"memcheck", Tool::Memcheck},
        {"ptrcheck", Tool::Ptrcheck},
        {"exp-ptrcheck", Tool::Ptrcheck},
        {"helgrind", Tool::Helgrind}
    };
    return theHash;
}

class Parser::Private
{
public:
    explicit Private(Parser *qq);

    void start();
    QString errorString;
    std::unique_ptr<QIODevice> m_device;

private:
    void parseError();
    QList<Frame> parseStack();
    Suppression parseSuppression();
    SuppressionFrame parseSuppressionFrame();
    Frame parseFrame();
    void parseStatus();
    void parseErrorCounts();
    void parseSuppressionCounts();
    void parseAnnounceThread();
    void checkProtocolVersion(const QString &versionStr);
    void checkTool(const QString &tool);
    XWhat parseXWhat();
    XauxWhat parseXauxWhat();
    int parseErrorKind(const QString &kind);

    void reportInternalError(const QString &errorString);
    QXmlStreamReader::TokenType blockingReadNext();
    bool notAtEnd() const;
    QString blockingReadElementText();

    Tool tool = Tool::Unknown;
    QXmlStreamReader reader;
    Parser *const q;
};

Parser::Private::Private(Parser *qq)
    : q(qq)
{
}

static quint64 parseHex(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toULongLong(&ok, 16);
    if (!ok)
        throw ParserException(Tr::tr("Could not parse hex number from \"%1\" (%2)").arg(str, context));
    return v;
}

static qint64 parseInt64(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toLongLong(&ok);
    if (!ok)
        throw ParserException(Tr::tr("Could not parse hex number from \"%1\" (%2)").arg(str, context));
    return v;
}

QXmlStreamReader::TokenType Parser::Private::blockingReadNext()
{
    QXmlStreamReader::TokenType token = QXmlStreamReader::Invalid;

    forever {
        token = reader.readNext();

        if (reader.error() == QXmlStreamReader::PrematureEndOfDocumentError) {
            if (reader.device()->waitForReadyRead(1000)) {
                // let's try again
                continue;
            } else {
                // device error, e.g. remote host closed connection, or timeout
                // ### we have no way to know if waitForReadyRead() timed out or failed with a real
                //     error, and sensible heuristics based on QIODevice fail.
                //     - error strings are translated and in any case not guaranteed to stay the same,
                //       so we can't use them.
                //     - errorString().isEmpty() does not work because errorString() is
                //       "network timeout error" if the waitFor... timed out.
                //     - isSequential() [for socket] && isOpen() doesn't work because isOpen()
                //       returns true if the remote host closed the connection.
                // ...so we fall back to knowing it might be a QAbstractSocket.

                QIODevice *dev = reader.device();
                auto sock = qobject_cast<const QAbstractSocket *>(dev);

                if (!sock || sock->state() != QAbstractSocket::ConnectedState)
                    throw ParserException(dev->errorString());
            }
        } else if (reader.hasError()) {
            throw ParserException(reader.errorString()); //TODO add line, column?
            break;
        } else {
            // read a valid next token
            break;
        }
    }

    return token;
}

bool Parser::Private::notAtEnd() const
{
    return !reader.atEnd()
           || reader.error() == QXmlStreamReader::PrematureEndOfDocumentError;
}

QString Parser::Private::blockingReadElementText()
{
    //analogous to QXmlStreamReader::readElementText(), but blocking. readElementText() doesn't recover from PrematureEndOfData,
    //but always returns a null string if isStartElement() is false (which is the case if it already parts of the text)
    //affects at least Qt <= 4.7.1. Reported as QTBUG-14661.

    if (!reader.isStartElement())
        throw ParserException(Tr::tr("trying to read element text although current position is not start of element"));

    QString result;

    forever {
        const QXmlStreamReader::TokenType type = blockingReadNext();
        switch (type) {
        case QXmlStreamReader::Characters:
        case QXmlStreamReader::EntityReference:
            result += reader.text();
            break;
        case QXmlStreamReader::EndElement:
            return result;
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::StartElement:
            throw ParserException(Tr::tr("Unexpected child element while reading element text"));
        default:
            //TODO handle
            throw ParserException(Tr::tr("Unexpected token type %1").arg(type));
            break;
        }
    }
    return QString();
}

void Parser::Private::checkProtocolVersion(const QString &versionStr)
{
    bool ok;
    const int version = versionStr.toInt(&ok);
    if (!ok)
        throw ParserException(Tr::tr("Could not parse protocol version from \"%1\"").arg(versionStr));
    if (version != 4)
        throw ParserException(Tr::tr("XmlProtocol version %1 not supported (supported version: 4)").arg(version));
}

void Parser::Private::checkTool(const QString &reportedStr)
{
    const auto reported = toolByName().constFind(reportedStr);
    if (reported == toolByName().constEnd())
        throw ParserException(Tr::tr("Valgrind tool \"%1\" not supported").arg(reportedStr));

    tool = reported.value();
}

XWhat Parser::Private::parseXWhat()
{
    XWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        const auto name = reader.name();
        if (name == QLatin1String("text"))
            what.text = blockingReadElementText();
        else if (name == QLatin1String("leakedbytes"))
            what.leakedbytes = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/leakedbytes");
        else if (name == QLatin1String("leakedblocks"))
            what.leakedblocks = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/leakedblocks");
        else if (name == QLatin1String("hthreadid"))
            what.hthreadid = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/hthreadid");
        else if (reader.isStartElement())
            reader.skipCurrentElement();
    }
    return what;
}

XauxWhat Parser::Private::parseXauxWhat()
{
    XauxWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        const auto name = reader.name();
        if (name == QLatin1String("text"))
            what.text = blockingReadElementText();
        else if (name == QLatin1String("file"))
            what.file = blockingReadElementText();
        else if (name == QLatin1String("dir"))
            what.dir = blockingReadElementText();
        else if (name == QLatin1String("line"))
            what.line = parseInt64(blockingReadElementText(), "error/xauxwhat/line");
        else if (name == QLatin1String("hthreadid"))
            what.hthreadid = parseInt64(blockingReadElementText(), "error/xauxwhat/hthreadid");
        else if (reader.isStartElement())
            reader.skipCurrentElement();
    }
    return what;
}

template <typename Enum>
int parseErrorEnum(const QString &kind)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<Enum>();
    const int value = metaEnum.keyToValue(kind.toUtf8());
    if (value >= 0)
        return value;
    throw ParserException(Tr::tr("Unknown %1 kind \"%2\"")
                              .arg(QString::fromUtf8(metaEnum.enumName()), kind));
}

int Parser::Private::parseErrorKind(const QString &kind)
{
    switch (tool) {
    case Tool::Memcheck:
        return parseErrorEnum<MemcheckError>(kind);
    case Tool::Ptrcheck:
        return parseErrorEnum<PtrcheckError>(kind);
    case Tool::Helgrind:
        return parseErrorEnum<HelgrindError>(kind);
    case Tool::Unknown:
    default:
        break;
    }
    throw ParserException(Tr::tr("Could not parse error kind, tool not yet set."));
}

static Status::State parseState(const QString &state)
{
    if (state == "RUNNING")
        return Status::Running;
    if (state == "FINISHED")
        return Status::Finished;
    throw ParserException(Tr::tr("Unknown state \"%1\"").arg(state));
}

static Stack makeStack(const XauxWhat &xauxwhat, const QList<Frame> &frames)
{
    Stack s;
    s.setFrames(frames);
    s.setFile(xauxwhat.file);
    s.setDirectory(xauxwhat.dir);
    s.setLine(xauxwhat.line);
    s.setHelgrindThreadId(xauxwhat.hthreadid);
    s.setAuxWhat(xauxwhat.text);
    return s;
}

void Parser::Private::parseError()
{
    Error e;
    QList<QList<Frame>> frames;
    XauxWhat currentAux;
    QList<XauxWhat> auxs;

    int lastAuxWhat = -1;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement())
            lastAuxWhat++;
        const auto name = reader.name();
        if (name == QLatin1String("unique")) {
            e.setUnique(parseHex(blockingReadElementText(), "unique"));
        } else if (name == QLatin1String("tid")) {
            e.setTid(parseInt64(blockingReadElementText(), "error/tid"));
        } else if (name == QLatin1String("kind")) { //TODO this is memcheck-specific:
            e.setKind(parseErrorKind(blockingReadElementText()));
        } else if (name == QLatin1String("suppression")) {
            e.setSuppression(parseSuppression());
        } else if (name == QLatin1String("xwhat")) {
            const XWhat xw = parseXWhat();
            e.setWhat(xw.text);
            e.setLeakedBlocks(xw.leakedblocks);
            e.setLeakedBytes(xw.leakedbytes);
            e.setHelgrindThreadId(xw.hthreadid);
        } else if (name == QLatin1String("what")) {
            e.setWhat(blockingReadElementText());
        } else if (name == QLatin1String("xauxwhat")) {
            if (!currentAux.text.isEmpty())
                auxs.push_back(currentAux);
            currentAux = parseXauxWhat();
        } else if (name == QLatin1String("auxwhat")) {
            const QString aux = blockingReadElementText();
            //concatenate multiple consecutive <auxwhat> tags
            if (lastAuxWhat > 1) {
                if (!currentAux.text.isEmpty())
                    auxs.push_back(currentAux);
                currentAux.clear();
                currentAux.text = aux;
            } else {
                if (!currentAux.text.isEmpty())
                    currentAux.text.append(' ');
                currentAux.text.append(aux);
            }
            lastAuxWhat = 0;
        } else if (name == QLatin1String("stack")) {
            frames.push_back(parseStack());
        } else if (reader.isStartElement()) {
            reader.skipCurrentElement();
        }
    }

    if (!currentAux.text.isEmpty())
        auxs.push_back(currentAux);

    //if we have less xaux/auxwhats than stacks, prepend empty xauxwhats
    //(the first frame usually has not xauxwhat in helgrind and memcheck)
    while (auxs.size() < frames.size())
        auxs.prepend(XauxWhat());

    //add empty stacks until sizes match
    while (frames.size() < auxs.size())
        frames.push_back({});

    QList<Stack> stacks;
    for (int i = 0; i < auxs.size(); ++i)
        stacks.append(makeStack(auxs[i], frames[i]));
    e.setStacks(stacks);

    emit q->error(e);
}

Frame Parser::Private::parseFrame()
{
    Frame frame;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            const auto name = reader.name();
            if (name == QLatin1String("ip"))
                frame.setInstructionPointer(parseHex(blockingReadElementText(), "error/frame/ip"));
            else if (name == QLatin1String("obj"))
                frame.setObject(blockingReadElementText());
            else if (name == QLatin1String("fn"))
                frame.setFunctionName( blockingReadElementText());
            else if (name == QLatin1String("dir"))
                frame.setDirectory(blockingReadElementText());
            else if (name == QLatin1String("file"))
                frame.setFileName(blockingReadElementText());
            else if (name == QLatin1String("line"))
                frame.setLine(parseInt64(blockingReadElementText(), "error/frame/line"));
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    return frame;
}

void Parser::Private::parseAnnounceThread()
{
    AnnounceThread at;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            const auto name = reader.name();
            if (name == QLatin1String("hthreadid"))
                at.setHelgrindThreadId(parseInt64(blockingReadElementText(), "announcethread/hthreadid"));
            else if (name == QLatin1String("stack"))
                at.setStack(parseStack());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    emit q->announceThread(at);
}

void Parser::Private::parseErrorCounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("pair")) {
                qint64 unique = 0;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (reader.isEndElement())
                        break;
                    if (reader.isStartElement()) {
                        const auto name = reader.name();
                        if (name == QLatin1String("unique"))
                            unique = parseHex(blockingReadElementText(), "errorcounts/pair/unique");
                        else if (name == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), "errorcounts/pair/count");
                        else if (reader.isStartElement())
                            reader.skipCurrentElement();
                    }
                }
                emit q->errorCount(unique, count);
            } else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }
}


void Parser::Private::parseSuppressionCounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("pair")) {
                QString pairName;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (reader.isEndElement())
                        break;
                    if (reader.isStartElement()) {
                        const auto name = reader.name();
                        if (name == QLatin1String("name"))
                            pairName = blockingReadElementText();
                        else if (name == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), "suppcounts/pair/count");
                        else if (reader.isStartElement())
                            reader.skipCurrentElement();
                    }
                }
                emit q->suppressionCount(pairName, count);
            } else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }
}

void Parser::Private::parseStatus()
{
    Status s;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            const auto name = reader.name();
            if (name == QLatin1String("state"))
                s.setState(parseState(blockingReadElementText()));
            else if (name == QLatin1String("time"))
                s.setTime(blockingReadElementText());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    emit q->status(s);
}

QList<Frame> Parser::Private::parseStack()
{
    QList<Frame> frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            if (reader.name() == QLatin1String("frame"))
                frames.append(parseFrame());
        }
    }

    return frames;
}

SuppressionFrame Parser::Private::parseSuppressionFrame()
{
    SuppressionFrame frame;

    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            const auto name = reader.name();
            if (name == QLatin1String("obj"))
                frame.setObject(blockingReadElementText());
            else if (name == QLatin1String("fun"))
                frame.setFunction( blockingReadElementText());
            else if (reader.isStartElement())
                reader.skipCurrentElement();
        }
    }

    return frame;
}

Suppression Parser::Private::parseSuppression()
{
    Suppression supp;
    SuppressionFrames frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (reader.isEndElement())
            break;
        if (reader.isStartElement()) {
            const auto name = reader.name();
            if (name == QLatin1String("sname"))
                supp.setName(blockingReadElementText());
            else if (name == QLatin1String("skind"))
                supp.setKind(blockingReadElementText());
            else if (name == QLatin1String("skaux"))
                supp.setAuxKind(blockingReadElementText());
            else if (name == QLatin1String("rawtext"))
                supp.setRawText(blockingReadElementText());
            else if (name == QLatin1String("sframe"))
                frames.push_back(parseSuppressionFrame());
        }
    }

    supp.setFrames(frames);
    return supp;
}

void Parser::Private::start()
{
    reader.setDevice(m_device.get());
    errorString.clear();

    bool success = true;
    try {
        while (notAtEnd()) {
            blockingReadNext();
            const auto name = reader.name();
            if (name == QLatin1String("error"))
                parseError();
            else if (name == QLatin1String("announcethread"))
                parseAnnounceThread();
            else if (name == QLatin1String("status"))
                parseStatus();
            else if (name == QLatin1String("errorcounts"))
                parseErrorCounts();
            else if (name == QLatin1String("suppcounts"))
                parseSuppressionCounts();
            else if (name == QLatin1String("protocolversion"))
                checkProtocolVersion(blockingReadElementText());
            else if (name == QLatin1String("protocoltool"))
                checkTool(blockingReadElementText());
        }
    } catch (const ParserException &e) {
        errorString = e.message();
        success = false;
    } catch (...) {
        errorString = Tr::tr("Unexpected exception caught during parsing.");
        success = false;
    }
    emit q->done(success, errorString);
}

Parser::Parser(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
}

Parser::~Parser()
{
    delete d;
}

QString Parser::errorString() const
{
    return d->errorString;
}

void Parser::setIODevice(QIODevice *device)
{
    QTC_ASSERT(device, return);
    QTC_ASSERT(device->isOpen(), return);
    d->m_device.reset(device);
}

void Parser::start()
{
    QTC_ASSERT(d->m_device, return);
    d->start();
}

} // namespace Valgrind::XmlProtocol
