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

#include <extensionsystem/pluginmanager.h>

#include <utils/async.h>
#include <utils/expected.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>

#include <QAbstractSocket>
#include <QHash>
#include <QMetaEnum>
#include <QMutex>
#include <QPromise>
#include <QWaitCondition>
#include <QXmlStreamReader>

using namespace Utils;

namespace Valgrind::XmlProtocol {

struct ParserException
{
    QString message;
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

enum class Tool { Unknown, Memcheck, Ptrcheck, Helgrind };

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

struct OutputData
{
    std::optional<Status> m_status = {};
    std::optional<Error> m_error = {};
    std::optional<QPair<qint64, qint64>> m_errorCount = {};
    std::optional<QPair<QString, qint64>> m_suppressionCount = {};
    std::optional<AnnounceThread> m_announceThread = {};
    std::optional<QString> m_internalError = {};
};

class ParserThread
{
public:
    // Called from the other thread, scheduled from the main thread through the queued
    // invocation.
    void run(QPromise<OutputData> &promise) {
        if (promise.isCanceled())
            return;
        m_promise = &promise;
        start();
        m_promise = nullptr;
    }

    // Called from the main thread exclusively
    void cancel()
    {
        QMutexLocker locker(&m_mutex);
        m_state = State::Canceled;
        m_waitCondition.wakeOne();
    }
    // Called from the main thread exclusively
    void finalize()
    {
        QMutexLocker locker(&m_mutex);
        if (m_state != State::Awaiting)
            return;
        m_state = State::Finalized;
        m_waitCondition.wakeOne();
    }
    // Called from the main thread exclusively
    void addData(const QByteArray &input)
    {
        if (input.isEmpty())
            return;
        QMutexLocker locker(&m_mutex);
        if (m_state != State::Awaiting)
            return;
        m_inputBuffer.append(input);
        m_waitCondition.wakeOne();
    }

private:
    // Called from the separate thread, exclusively by run(). Checks if the new data already
    // came before sleeping with wait condition. If so, it doesn't sleep with wait condition,
    // but returns the data collected in meantime. Otherwise, it calls wait() on wait condition.
    expected_str<QByteArray> waitForData()
    {
        QMutexLocker locker(&m_mutex);
        while (true) {
            if (m_state == State::Canceled)
                return make_unexpected(Tr::tr("Parsing canceled."));
            if (!m_inputBuffer.isEmpty())
                return std::exchange(m_inputBuffer, {});
            if (m_state == State::Finalized)
                return make_unexpected(Tr::tr("Premature end of XML document."));
            m_waitCondition.wait(&m_mutex);
        }
        QTC_CHECK(false);
        return {};
    }

    void start();
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

    QXmlStreamReader::TokenType blockingReadNext();
    bool notAtEnd() const;
    QString blockingReadElementText();

    void emitStatus(const Status &status) { m_promise->addResult(OutputData{{status}}); }
    void emitError(const Error &error) { m_promise->addResult(OutputData{{}, {error}}); }
    void emitErrorCount(qint64 unique, qint64 count) {
        m_promise->addResult(OutputData{{}, {}, {{unique, count}}});
    }
    void emitSuppressionCount(const QString &name, qint64 count) {
        m_promise->addResult(OutputData{{}, {}, {}, {{name, count}}});
    }
    void emitAnnounceThread(const AnnounceThread &announceThread) {
        m_promise->addResult(OutputData{{}, {}, {}, {}, {announceThread}});
    }
    void emitInternalError(const QString &errorString) {
        m_promise->addResult(OutputData{{}, {}, {}, {}, {}, {errorString}});
    }

    enum class State { Awaiting, Finalized, Canceled };

    Tool m_tool = Tool::Unknown; // Accessed only from the other thread.
    QXmlStreamReader m_reader; // Accessed only from the other thread.

    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    QPromise<OutputData> *m_promise = nullptr;
    State m_state = State::Awaiting; // Accessed from both threads, needs mutex.
    QByteArray m_inputBuffer; // Accessed from both threads, needs mutex.
};

static quint64 parseHex(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toULongLong(&ok, 16);
    if (!ok)
        throw ParserException{Tr::tr("Could not parse hex number from \"%1\" (%2)").arg(str, context)};
    return v;
}

static qint64 parseInt64(const QString &str, const QString &context)
{
    bool ok;
    const quint64 v = str.toLongLong(&ok);
    if (!ok)
        throw ParserException{Tr::tr("Could not parse hex number from \"%1\" (%2).").arg(str, context)};
    return v;
}

QXmlStreamReader::TokenType ParserThread::blockingReadNext()
{
    QXmlStreamReader::TokenType token = QXmlStreamReader::Invalid;
    while (true) {
        token = m_reader.readNext();
        if (m_reader.error() == QXmlStreamReader::PrematureEndOfDocumentError) {
            const auto data = waitForData();
            if (data) {
                m_reader.addData(*data);
                continue;
            } else {
                throw ParserException{data.error()};
            }
        } else if (m_reader.hasError()) {
            throw ParserException{m_reader.errorString()}; //TODO add line, column?
            break;
        } else {
            // read a valid next token
            break;
        }
    }
    return token;
}

bool ParserThread::notAtEnd() const
{
    return !m_reader.atEnd() || m_reader.error() == QXmlStreamReader::PrematureEndOfDocumentError;
}

QString ParserThread::blockingReadElementText()
{
    //analogous to QXmlStreamReader::readElementText(), but blocking. readElementText() doesn't recover from PrematureEndOfData,
    //but always returns a null string if isStartElement() is false (which is the case if it already parts of the text)
    //affects at least Qt <= 4.7.1. Reported as QTBUG-14661.

    if (!m_reader.isStartElement())
        throw ParserException{Tr::tr("Trying to read element text although current position is not start of element.")};

    QString result;

    while (true) {
        const QXmlStreamReader::TokenType type = blockingReadNext();
        switch (type) {
        case QXmlStreamReader::Characters:
        case QXmlStreamReader::EntityReference:
            result += m_reader.text();
            break;
        case QXmlStreamReader::EndElement:
            return result;
        case QXmlStreamReader::ProcessingInstruction:
        case QXmlStreamReader::Comment:
            break;
        case QXmlStreamReader::StartElement:
            throw ParserException{Tr::tr("Unexpected child element while reading element text")};
        default:
            //TODO handle
            throw ParserException{Tr::tr("Unexpected token type %1").arg(type)};
            break;
        }
    }
    return {};
}

void ParserThread::checkProtocolVersion(const QString &versionStr)
{
    bool ok;
    const int version = versionStr.toInt(&ok);
    if (!ok)
        throw ParserException{Tr::tr("Could not parse protocol version from \"%1\"").arg(versionStr)};
    if (version != 4)
        throw ParserException{Tr::tr("XmlProtocol version %1 not supported (supported version: 4)").arg(version)};
}

void ParserThread::checkTool(const QString &reportedStr)
{
    const auto reported = toolByName().constFind(reportedStr);
    if (reported == toolByName().constEnd())
        throw ParserException{Tr::tr("Valgrind tool \"%1\" not supported").arg(reportedStr)};
    m_tool = reported.value();
}

XWhat ParserThread::parseXWhat()
{
    XWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        const auto name = m_reader.name();
        if (name == QLatin1String("text"))
            what.text = blockingReadElementText();
        else if (name == QLatin1String("leakedbytes"))
            what.leakedbytes = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/leakedbytes");
        else if (name == QLatin1String("leakedblocks"))
            what.leakedblocks = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/leakedblocks");
        else if (name == QLatin1String("hthreadid"))
            what.hthreadid = parseInt64(blockingReadElementText(), "error/xwhat[memcheck]/hthreadid");
        else if (m_reader.isStartElement())
            m_reader.skipCurrentElement();
    }
    return what;
}

XauxWhat ParserThread::parseXauxWhat()
{
    XauxWhat what;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        const auto name = m_reader.name();
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
        else if (m_reader.isStartElement())
            m_reader.skipCurrentElement();
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
    throw ParserException{Tr::tr("Unknown %1 kind \"%2\"")
                              .arg(QString::fromUtf8(metaEnum.enumName()), kind)};
}

int ParserThread::parseErrorKind(const QString &kind)
{
    switch (m_tool) {
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
    throw ParserException{Tr::tr("Could not parse error kind, tool not yet set.")};
}

static Status::State parseState(const QString &state)
{
    if (state == "RUNNING")
        return Status::Running;
    if (state == "FINISHED")
        return Status::Finished;
    throw ParserException{Tr::tr("Unknown state \"%1\"").arg(state)};
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

void ParserThread::parseError()
{
    Error e;
    QList<QList<Frame>> frames;
    XauxWhat currentAux;
    QList<XauxWhat> auxs;
    int lastAuxWhat = -1;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement())
            lastAuxWhat++;
        const auto name = m_reader.name();
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
        } else if (m_reader.isStartElement()) {
            m_reader.skipCurrentElement();
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

    emitError(e);
}

Frame ParserThread::parseFrame()
{
    Frame frame;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            const auto name = m_reader.name();
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
            else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
    return frame;
}

void ParserThread::parseAnnounceThread()
{
    AnnounceThread at;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            const auto name = m_reader.name();
            if (name == QLatin1String("hthreadid"))
                at.setHelgrindThreadId(parseInt64(blockingReadElementText(), "announcethread/hthreadid"));
            else if (name == QLatin1String("stack"))
                at.setStack(parseStack());
            else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
    emitAnnounceThread(at);
}

void ParserThread::parseErrorCounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QLatin1String("pair")) {
                qint64 unique = 0;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (m_reader.isEndElement())
                        break;
                    if (m_reader.isStartElement()) {
                        const auto name = m_reader.name();
                        if (name == QLatin1String("unique"))
                            unique = parseHex(blockingReadElementText(), "errorcounts/pair/unique");
                        else if (name == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), "errorcounts/pair/count");
                        else if (m_reader.isStartElement())
                            m_reader.skipCurrentElement();
                    }
                }
                emitErrorCount(unique, count);
            } else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
}

void ParserThread::parseSuppressionCounts()
{
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QLatin1String("pair")) {
                QString pairName;
                qint64 count = 0;
                while (notAtEnd()) {
                    blockingReadNext();
                    if (m_reader.isEndElement())
                        break;
                    if (m_reader.isStartElement()) {
                        const auto name = m_reader.name();
                        if (name == QLatin1String("name"))
                            pairName = blockingReadElementText();
                        else if (name == QLatin1String("count"))
                            count = parseInt64(blockingReadElementText(), "suppcounts/pair/count");
                        else if (m_reader.isStartElement())
                            m_reader.skipCurrentElement();
                    }
                }
                emitSuppressionCount(pairName, count);
            } else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
}

void ParserThread::parseStatus()
{
    Status s;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            const auto name = m_reader.name();
            if (name == QLatin1String("state"))
                s.setState(parseState(blockingReadElementText()));
            else if (name == QLatin1String("time"))
                s.setTime(blockingReadElementText());
            else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
    emitStatus(s);
}

QList<Frame> ParserThread::parseStack()
{
    QList<Frame> frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            if (m_reader.name() == QLatin1String("frame"))
                frames.append(parseFrame());
        }
    }
    return frames;
}

SuppressionFrame ParserThread::parseSuppressionFrame()
{
    SuppressionFrame frame;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            const auto name = m_reader.name();
            if (name == QLatin1String("obj"))
                frame.setObject(blockingReadElementText());
            else if (name == QLatin1String("fun"))
                frame.setFunction( blockingReadElementText());
            else if (m_reader.isStartElement())
                m_reader.skipCurrentElement();
        }
    }
    return frame;
}

Suppression ParserThread::parseSuppression()
{
    Suppression supp;
    SuppressionFrames frames;
    while (notAtEnd()) {
        blockingReadNext();
        if (m_reader.isEndElement())
            break;
        if (m_reader.isStartElement()) {
            const auto name = m_reader.name();
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

void ParserThread::start()
{
    try {
        while (notAtEnd()) {
            blockingReadNext();
            const auto name = m_reader.name();
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
        emitInternalError(e.message);
    } catch (...) {
        emitInternalError(Tr::tr("Unexpected exception caught during parsing."));
    }
}

class ParserPrivate
{
public:
    ParserPrivate(Parser *parser)
        : q(parser)
    {}

    ~ParserPrivate()
    {
        if (!m_watcher)
            return;
        m_thread->cancel();
        ExtensionSystem::PluginManager::futureSynchronizer()->addFuture(m_watcher->future());
    }

    void start()
    {
        QTC_ASSERT(!m_watcher, return);
        QTC_ASSERT(m_socket || !m_data.isEmpty(), return);

        m_errorString = {};
        m_thread.reset(new ParserThread);
        m_watcher.reset(new QFutureWatcher<OutputData>);
        QObject::connect(m_watcher.get(), &QFutureWatcherBase::resultReadyAt, q, [this](int index) {
            const OutputData data = m_watcher->resultAt(index);
            if (data.m_status)
                emit q->status(*data.m_status);
            if (data.m_error)
                emit q->error(*data.m_error);
            if (data.m_errorCount)
                emit q->errorCount(data.m_errorCount->first, data.m_errorCount->second);
            if (data.m_suppressionCount)
                emit q->suppressionCount(data.m_suppressionCount->first, data.m_suppressionCount->second);
            if (data.m_announceThread)
                emit q->announceThread(*data.m_announceThread);
            if (data.m_internalError)
                m_errorString = data.m_internalError;
        });
        QObject::connect(m_watcher.get(), &QFutureWatcherBase::finished, q, [this] {
            emit q->done(!m_errorString, m_errorString.value_or(QString()));
            m_watcher.release()->deleteLater();
            m_thread.reset();
            m_socket.reset();
        });
        if (m_socket) {
            QObject::connect(m_socket.get(), &QIODevice::readyRead, q, [this] {
                if (m_thread)
                    m_thread->addData(m_socket->readAll());
            });
            QObject::connect(m_socket.get(), &QAbstractSocket::disconnected, q, [this] {
                if (m_thread)
                    m_thread->finalize();
            });
            m_thread->addData(m_socket->readAll());
        } else {
            m_thread->addData(m_data);
            m_thread->finalize();
        }
        auto parse = [](QPromise<OutputData> &promise, const std::shared_ptr<ParserThread> &thread) {
            thread->run(promise);
        };
        m_watcher->setFuture(Utils::asyncRun(parse, m_thread));
    }

    Parser *q = nullptr;

    QByteArray m_data;
    std::unique_ptr<QAbstractSocket> m_socket;
    std::unique_ptr<QFutureWatcher<OutputData>> m_watcher;
    std::shared_ptr<ParserThread> m_thread;
    std::optional<QString> m_errorString;
};

Parser::Parser(QObject *parent)
    : QObject(parent)
    , d(new ParserPrivate(this))
{}

Parser::~Parser() = default;

QString Parser::errorString() const
{
    return d->m_errorString.value_or(QString());
}

void Parser::setSocket(QAbstractSocket *socket)
{
    QTC_ASSERT(socket, return);
    QTC_ASSERT(socket->isOpen(), return);
    QTC_ASSERT(!isRunning(), return);
    socket->setParent(nullptr); // Don't delete it together with parent QTcpServer anymore.
    d->m_socket.reset(socket);
}

void Parser::setData(const QByteArray &data)
{
    QTC_ASSERT(!isRunning(), return);
    d->m_data = data;
}

void Parser::start()
{
    d->start();
}

bool Parser::isRunning() const
{
    return d->m_watcher.get();
}

bool Parser::runBlocking()
{
    bool ok = false;
    QEventLoop loop;

    const auto finalize = [&loop, &ok](bool success) {
        ok = success;
        // Refer to the QObject::deleteLater() docs.
        QMetaObject::invokeMethod(&loop, [&loop] { loop.quit(); }, Qt::QueuedConnection);
    };

    connect(this, &Parser::done, &loop, finalize);
    QTimer::singleShot(0, this, &Parser::start);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    return ok;
}

} // namespace Valgrind::XmlProtocol
