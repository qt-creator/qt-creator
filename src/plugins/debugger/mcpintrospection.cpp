// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commonoptionspage.h"
#include "debuggerengine.h"
#include "enginemanager.h"
#include "mcpsessionstate.h"
#include "mcpsupport_p.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "threadshandler.h"

#include <mcp/server/toolregistry.h>

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>

#include <algorithm>
#include <limits>
#include <optional>

using namespace Utils;

namespace Debugger::Internal {

enum {
    defaultMemoryTimeoutMs = 10000,
    defaultRegisterTimeoutMs = 5000,
    defaultThreadTimeoutMs = 5000,
    defaultMaxFrames = 10,
    maximalMaxFrames = 200,
    defaultMaxThreads = 64,
    maximalMaxThreads = 256,
    defaultMaxTotalFrames = 500,
    maximalMaxTotalFrames = 5000
};

static const QStringList &frameFields()
{
    static const QStringList fields{"function", "file", "line", "address", "module"};
    return fields;
}

static int boundedArgument(const QJsonObject &args, const QString &name, int defaultValue,
                           int minimum, int maximum)
{
    return std::clamp(args.value(name).toInt(defaultValue), minimum, maximum);
}

static QJsonObject frameToJson(const StackFrame &frame, int level, const QSet<QString> &fields)
{
    QJsonObject obj{{"level", level}};
    if (fields.contains("function") && !frame.function.isEmpty())
        obj["function"] = frame.function;
    if (fields.contains("file") && !frame.file.isEmpty())
        obj["file"] = frame.file.toUserOutput();
    if (fields.contains("line") && frame.line >= 0)
        obj["line"] = frame.line;
    if (fields.contains("address") && frame.address != 0)
        obj["address"] = QString("0x%1").arg(frame.address, 0, 16);
    if (fields.contains("module") && !frame.module.isEmpty())
        obj["module"] = frame.module;
    return obj;
}

static QJsonObject threadToJson(const ThreadData &data)
{
    QJsonObject obj{{"id", data.id}};
    if (!data.name.isEmpty())
        obj["name"] = data.name;
    if (!data.state.isEmpty())
        obj["state"] = data.state;
    if (!data.targetId.isEmpty())
        obj["target_id"] = data.targetId;
    return obj;
}

/*
    Collects the call stacks of several threads of a stopped session. The
    engines load the stack of the selected thread only, so each thread is
    selected in turn and its stack waited for. The selection the user had is
    put back afterwards. Nothing here resumes the target, but something else
    may, and then collection stops: stacks from different stops are not
    presented as one picture.
*/
class ThreadStackCollector final : public QObject
{
public:
    class Options
    {
    public:
        QStringList threadIds;
        QSet<QString> fields;
        int maxFrames = defaultMaxFrames;
        int maxThreads = defaultMaxThreads;
        int maxTotalFrames = defaultMaxTotalFrames;
        int threadTimeoutMs = defaultThreadTimeoutMs;
    };

    ThreadStackCollector(McpSessionState *state, const Options &options, const McpReply &reply)
        : m_state(state)
        , m_options(options)
        , m_reply(reply)
        , m_stopId(state->stopId())
        , m_originalThreadId(state->currentThreadId())
        , m_originalFrameLevel(state->currentFrameLevel())
    {}

    void start()
    {
        ThreadsHandler *threads = m_state->engine()->threadsHandler();
        QStringList available;
        threads->forItemsAtLevel<1>([&](ThreadItem *item) {
            available.append(item->id());
            m_threadData.insert(item->id(), item->threadData);
        });

        const QStringList wanted = m_options.threadIds.isEmpty() ? available
                                                                 : m_options.threadIds;
        for (const QString &id : wanted) {
            if (!available.contains(id)) {
                m_results.append(QJsonObject{{"id", id},
                                             {"error", "The debugger reports no such thread."}});
            } else if (m_pending.size() < m_options.maxThreads) {
                m_pending.append(id);
            } else {
                m_omitted.append(id);
            }
        }
        next();
    }

private:
    bool stillAtTheSameStop() const
    {
        return m_state && m_state->isStopped() && m_state->stopId() == m_stopId;
    }

    void next()
    {
        if (!stillAtTheSameStop()) {
            m_targetResumed = true;
            for (const QString &id : std::as_const(m_pending)) {
                QJsonObject obj = threadToJson(m_threadData.value(id));
                obj["error"] = "Not collected: the target resumed or the session ended.";
                m_results.append(obj);
            }
            m_pending.clear();
            finish();
            return;
        }
        if (m_pending.isEmpty()) {
            restoreThread();
            return;
        }

        const QString id = m_pending.takeFirst();
        if (m_state->currentThreadId() == id && m_state->isStackReady()) {
            collect(id);
            next();
            return;
        }

        DebuggerEngine *engine = m_state->engine();
        const Thread thread = engine->threadsHandler()->threadForId(id);
        QTC_ASSERT(thread, next(); return);
        engine->threadsHandler()->setCurrentThread(thread);
        engine->selectThread(thread);
        const QPointer<McpSessionState> state = m_state;
        waitUntil(m_state, m_options.threadTimeoutMs,
                  [state, id] {
                      return state && state->currentThreadId() == id && state->isStackReady();
                  },
                  [this, id](WaitOutcome outcome) {
                      if (outcome == WaitOutcome::Ready) {
                          collect(id);
                      } else if (stillAtTheSameStop()) {
                          QJsonObject obj = threadToJson(m_threadData.value(id));
                          obj["error"] = "The call stack of this thread did not arrive in time.";
                          m_results.append(obj);
                      } else {
                          m_pending.prepend(id);
                      }
                      next();
                  });
    }

    void collect(const QString &id)
    {
        const StackHandler *stack = m_state->engine()->stackHandler();
        QJsonObject obj = threadToJson(m_threadData.value(id));
        const int available = stack->stackSize();
        const int budget = std::max(0, m_options.maxTotalFrames - m_totalFrames);
        const int count = std::min({available, m_options.maxFrames, budget});
        QJsonArray frames;
        for (int i = 0; i < count; ++i)
            frames.append(frameToJson(stack->frameAt(i), i, m_options.fields));
        m_totalFrames += count;
        obj["frames"] = frames;
        obj["frames_loaded"] = available;
        const bool more = available > count || stack->canExpand();
        obj["truncated"] = more;
        if (more) {
            if (count == budget && budget < std::min(available, m_options.maxFrames))
                obj["truncation_reason"] = "max_total_frames";
            else if (available > count)
                obj["truncation_reason"] = "max_frames";
            else
                obj["truncation_reason"] = "the debugger loaded only part of the stack";
        }
        m_results.append(obj);
    }

    void restoreThread()
    {
        if (m_originalThreadId.isEmpty() || m_state->currentThreadId() == m_originalThreadId) {
            restoreFrame();
            return;
        }
        DebuggerEngine *engine = m_state->engine();
        const Thread thread = engine->threadsHandler()->threadForId(m_originalThreadId);
        if (!thread) {
            m_restoreProblem = "The originally selected thread no longer exists.";
            finish();
            return;
        }
        engine->threadsHandler()->setCurrentThread(thread);
        engine->selectThread(thread);
        const QPointer<McpSessionState> state = m_state;
        const QString id = m_originalThreadId;
        waitUntil(m_state, m_options.threadTimeoutMs,
                  [state, id] {
                      return state && state->currentThreadId() == id && state->isStackReady();
                  },
                  [this](WaitOutcome outcome) {
                      if (outcome != WaitOutcome::Ready) {
                          m_restoreProblem = "The originally selected thread could not be "
                                             "selected again in time.";
                          finish();
                          return;
                      }
                      restoreFrame();
                  });
    }

    void restoreFrame()
    {
        if (!stillAtTheSameStop()) {
            m_targetResumed = true;
            finish();
            return;
        }
        if (m_originalFrameLevel <= 0 || m_state->currentFrameLevel() == m_originalFrameLevel) {
            finish();
            return;
        }
        if (m_originalFrameLevel >= m_state->engine()->stackHandler()->stackSize()) {
            m_restoreProblem = "The originally selected frame is no longer on the stack.";
            finish();
            return;
        }
        m_state->engine()->activateFrame(m_originalFrameLevel);
        const QPointer<McpSessionState> state = m_state;
        const int level = m_originalFrameLevel;
        waitUntil(m_state, m_options.threadTimeoutMs,
                  [state, level] { return state && state->currentFrameLevel() == level; },
                  [this](WaitOutcome outcome) {
                      if (outcome != WaitOutcome::Ready)
                          m_restoreProblem = "The originally selected frame could not be "
                                             "selected again in time.";
                      finish();
                  });
    }

    void finish()
    {
        QJsonObject result{{"threads", m_results},
                           {"stop_id", m_stopId},
                           {"same_stop", !m_targetResumed && stillAtTheSameStop()},
                           {"target_resumed", m_targetResumed},
                           {"selection_restored", m_restoreProblem.isEmpty() && !m_targetResumed},
                           {"note", "The threads were read one after another while the target "
                                    "stayed stopped. This is not an atomic snapshot of all CPUs "
                                    "or of target memory, and lists only the threads the "
                                    "debugger reports."}};
        if (!m_restoreProblem.isEmpty())
            result["selection_problem"] = m_restoreProblem;
        if (!m_omitted.isEmpty())
            result["omitted_threads"] = QJsonArray::fromStringList(m_omitted);
        if (m_state)
            result["context"] = m_state->context();
        const McpReply reply = m_reply;
        deleteLater();
        reply(result);
    }

    QPointer<McpSessionState> m_state;
    Options m_options;
    McpReply m_reply;
    int m_stopId = 0;
    QString m_originalThreadId;
    int m_originalFrameLevel = -1;
    QHash<QString, ThreadData> m_threadData;
    QStringList m_pending;
    QStringList m_omitted;
    QJsonArray m_results;
    int m_totalFrames = 0;
    bool m_targetResumed = false;
    QString m_restoreProblem;
};

static void collectThreadStacks(const QJsonObject &args, const McpReply &reply)
{
    const Result<McpSessionState *> state = pausedSessionState();
    if (!state) {
        reply(ResultError(state.error()));
        return;
    }

    ThreadStackCollector::Options options;
    for (const QJsonValue &id : args.value("thread_ids").toArray())
        options.threadIds.append(id.toString());
    const QJsonArray fields = args.value("fields").toArray();
    for (const QJsonValue &field : fields)
        options.fields.insert(field.toString());
    if (fields.isEmpty()) {
        for (const QString &field : frameFields())
            options.fields.insert(field);
    }
    options.maxFrames = boundedArgument(args, "max_frames", defaultMaxFrames, 1, maximalMaxFrames);
    options.maxThreads = boundedArgument(args, "max_threads", defaultMaxThreads, 1,
                                         maximalMaxThreads);
    options.maxTotalFrames = boundedArgument(args, "max_total_frames", defaultMaxTotalFrames, 1,
                                             maximalMaxTotalFrames);
    options.threadTimeoutMs = timeoutArgument(args, defaultThreadTimeoutMs);

    (new ThreadStackCollector(*state, options, reply))->start();
}

// Registers

static QString registerKindName(RegisterKind kind)
{
    switch (kind) {
    case IntegerRegister: return "integer";
    case FloatRegister:   return "float";
    case VectorRegister:  return "vector";
    case FlagRegister:    return "flags";
    case OtherRegister:   return "other";
    default:              return "unknown";
    }
}

// All bits of the value in hex, as wide as the register, most significant
// digit first. Without a known size, all significant digits.
static QString losslessHex(const RegisterValue &value, int size)
{
    const int capacity = 16 * RegisterValue::Lanes;
    const int bytes = size > 0 ? std::min(size, capacity) : capacity;
    QString hex;
    for (int i = bytes - 1; i >= 0; --i) {
        const Quint128 &lane = value.v.u128[i / 16];
        const quint64 word = (i % 16) < 8 ? lane.lo : lane.hi;
        const uint byte = (word >> (8 * (i % 8))) & 0xff;
        hex += QString("%1").arg(byte, 2, 16, QLatin1Char('0'));
    }
    if (size <= 0) {
        while (hex.size() > 1 && hex.startsWith('0'))
            hex.remove(0, 1);
    }
    return "0x" + hex;
}

/*!
    \internal

    Describes \a reg for debugger_get_registers. \a reportedAtThisStop says
    whether the engine reported it with the last update.
*/
QJsonObject mcpRegisterToJson(const Register &reg, bool reportedAtThisStop)
{
    QJsonObject obj{{"name", reg.name},
                    {"kind", registerKindName(reg.kind)},
                    {"groups", QJsonArray::fromStringList(reg.groups)}};
    if (reg.size > 0)
        obj["bit_width"] = reg.size * 8;
    if (!reg.reportedType.isEmpty())
        obj["type"] = reg.reportedType;
    if (!reportedAtThisStop) {
        obj["available"] = false;
        obj["reason"] = "The debugger did not report this register at this stop.";
    } else if (!reg.value.known) {
        obj["available"] = false;
        obj["reason"] = "The debugger reported no value for this register.";
    } else if (reg.size > int(sizeof(reg.value.v))) {
        obj["available"] = false;
        obj["reason"] = QString("The register is %1 bits wide, and Qt Creator holds at most %2 "
                                "bits of a register value.")
                            .arg(reg.size * 8).arg(sizeof(reg.value.v) * 8);
    } else {
        obj["available"] = true;
        obj["value"] = losslessHex(reg.value, reg.size);
    }
    return obj;
}

static void getRegisters(const QJsonObject &args, const McpReply &reply)
{
    const Result<McpSessionState *> state = pausedSessionState();
    if (!state) {
        reply(ResultError(state.error()));
        return;
    }
    DebuggerEngine *engine = (*state)->engine();
    if (!engine->hasCapability(RegisterCapability)) {
        reply(ResultError(QString("The %1 debugger of this session does not provide access to "
                                  "registers.").arg(engine->debuggerName())));
        return;
    }

    QStringList names;
    for (const QJsonValue &name : args.value("names").toArray())
        names.append(name.toString());
    const QString group = args.value("group").toString();

    const QPointer<McpSessionState> session = *state;
    session->expectRegisters();
    engine->reloadRegistersEvenIfHidden();
    waitUntil(session, timeoutArgument(args, defaultRegisterTimeoutMs),
              [session] { return session && session->isRegistersReady(); },
              [session, names, group, reply](WaitOutcome outcome) {
        if (outcome != WaitOutcome::Ready) {
            reply(ResultError(notReadyMessage("registers", session,
                                              outcome == WaitOutcome::SessionEnded)));
            return;
        }
        const RegisterHandler *handler = session->engine()->registerHandler();
        const QList<Register> all = handler->registers();

        QStringList groups;
        for (const Register &reg : all) {
            for (const QString &g : reg.groups) {
                if (!groups.contains(g))
                    groups.append(g);
            }
        }
        if (!group.isEmpty() && !groups.contains(group)) {
            reply(ResultError(QString("The debugger reports no register group \"%1\". Its "
                                      "groups are: %2.").arg(group, groups.join(", "))));
            return;
        }

        QJsonArray registers;
        QStringList found;
        for (const Register &reg : all) {
            if (!names.isEmpty() && !names.contains(reg.name))
                continue;
            if (!group.isEmpty() && !reg.groups.contains(group))
                continue;
            found.append(reg.name);
            registers.append(mcpRegisterToJson(reg, handler->wasUpdatedByLastCommit(reg.name)));
        }

        const int frameLevel = session->currentFrameLevel();
        QJsonObject result{{"registers", registers},
                           {"groups", QJsonArray::fromStringList(groups)},
                           {"frame_level", frameLevel},
                           {"context", session->context()}};
        const std::optional<bool> followsFrame
            = session->engine()->registersFollowSelectedFrame();
        if (frameLevel <= 0 || followsFrame == false) {
            result["values_for_frame_level"] = 0;
            result["live_cpu_values"] = true;
            if (frameLevel > 0) {
                result["note"] = "This debugger reports the registers of the innermost frame, "
                                 "whichever frame is selected.";
            }
        } else if (followsFrame == true) {
            result["values_for_frame_level"] = frameLevel;
            result["live_cpu_values"] = false;
            result["note"] = "The values are those the debugger reconstructs for the selected "
                             "frame. Registers it cannot reconstruct for an outer frame may hold "
                             "the current CPU value.";
        } else {
            result["note"] = "Whether this debugger reports the registers of the selected frame "
                             "or of the innermost one is not known.";
        }
        QStringList missing;
        for (const QString &name : std::as_const(names)) {
            if (!found.contains(name))
                missing.append(name);
        }
        if (!missing.isEmpty())
            result["unknown_names"] = QJsonArray::fromStringList(missing);
        reply(result);
    });
}

// Memory

class AddressRange
{
public:
    quint64 first = 0;
    quint64 last = 0;
};

static Result<quint64> parseAddress(const QString &text)
{
    bool ok = false;
    const quint64 value = text.trimmed().toULongLong(&ok, 0);
    if (!ok)
        return ResultError(QString("\"%1\" is not an address.").arg(text.trimmed()));
    return value;
}

static Result<QList<AddressRange>> parseAddressRanges(const QString &text)
{
    QList<AddressRange> ranges;
    for (const QString &part : text.split(',', Qt::SkipEmptyParts)) {
        const QStringList bounds = part.split('-');
        if (bounds.size() != 2)
            return ResultError(QString("\"%1\" is not a range of the form first-last.").arg(part.trimmed()));
        const Result<quint64> first = parseAddress(bounds.at(0));
        if (!first)
            return ResultError(first.error());
        const Result<quint64> last = parseAddress(bounds.at(1));
        if (!last)
            return ResultError(last.error());
        if (*last < *first)
            return ResultError(QString("The range \"%1\" ends before it starts.").arg(part.trimmed()));
        ranges.append({*first, *last});
    }
    return ranges;
}

// Whether the session debugs an application on an operating system, whose
// memory is ordinary process memory. A bare metal target, or one of unknown
// kind, may have memory-mapped I/O behind any address.
static bool debugsProcessMemory(const DebuggerRunParameters &rp)
{
    const ProjectExplorer::Abi::OS os = rp.toolChainAbi().os();
    if (os == ProjectExplorer::Abi::BareMetalOS)
        return false;
    switch (rp.startMode()) {
    case StartInternal:
    case StartExternal:
    case AttachToLocalProcess:
    case AttachToCrashedProcess:
    case AttachToCore:
    case AttachToRemoteProcess:
    case StartRemoteProcess:
    case AttachToIosDevice:
        return true;
    case AttachToRemoteServer:
        return os != ProjectExplorer::Abi::UnknownOS;
    default:
        return false;
    }
}

/*!
    \internal

    Says why reading \a length bytes from \a address in a session started
    with \a rp is not allowed, or on what grounds it is.
*/
Result<QString> mcpMemoryAccessGrant(const DebuggerRunParameters &rp, quint64 address,
                                     quint64 length)
{
    const CommonSettings &s = commonSettings();
    switch (s.mcpMemoryAccess()) {
    case CommonSettings::McpMemoryNever:
        return ResultError("Reading memory over MCP is turned off in Preferences > Debugger > "
                           "General > MCP Server.");
    case CommonSettings::McpMemoryAnywhere:
        return QString("any_memory");
    default:
        break;
    }
    if (debugsProcessMemory(rp))
        return QString("process_memory");

    const Result<QList<AddressRange>> ranges = parseAddressRanges(s.mcpMemoryRanges());
    if (!ranges) {
        return ResultError("The readable ranges in Preferences > Debugger > General > MCP Server "
                           "cannot be read: " + ranges.error());
    }
    const quint64 last = address + length - 1;
    for (const AddressRange &range : *ranges) {
        if (address >= range.first && last <= range.last)
            return QString("allowed_range");
    }
    return ResultError(QString("This session debugs a bare metal target, or one of unknown kind, "
                               "where an address may be memory-mapped I/O. Reading 0x%1-0x%2 "
                               "needs a readable range that covers it in Preferences > Debugger > "
                               "General > MCP Server, or \"Any memory\" there.")
                           .arg(address, 0, 16).arg(last, 0, 16));
}

static QJsonObject memorySegment(const MemoryReadResult &result, quint64 offset, quint64 length,
                                 const QString &encoding, const QString &reason)
{
    QJsonObject segment{{"address", QString("0x%1").arg(result.address + offset, 0, 16)},
                        {"offset", qint64(offset)},
                        {"length", qint64(length)},
                        {"readable", reason.isNull()}};
    if (reason.isNull()) {
        const QByteArray bytes = result.data.mid(qsizetype(offset), qsizetype(length));
        segment["data"] = QString::fromLatin1(encoding == "base64" ? bytes.toBase64()
                                                                   : bytes.toHex());
    } else {
        segment["reason"] = reason;
    }
    return segment;
}

static void readMemory(const QJsonObject &args, const McpReply &reply)
{
    const Result<McpSessionState *> state = pausedSessionState();
    if (!state) {
        reply(ResultError(state.error()));
        return;
    }
    DebuggerEngine *engine = (*state)->engine();
    if (!engine->canReadMemory()) {
        reply(ResultError(QString("The %1 debugger of this session cannot read memory.")
                              .arg(engine->debuggerName())));
        return;
    }

    const QJsonValue addressValue = args.value("address");
    const Result<quint64> address = addressValue.isString()
                                        ? parseAddress(addressValue.toString())
                                        : Result<quint64>(quint64(addressValue.toInteger(-1)));
    if (!address || (!addressValue.isString() && addressValue.toInteger(-1) < 0)) {
        reply(ResultError(address ? QString("\"address\" must not be negative.") : address.error()));
        return;
    }
    const qint64 requested = args.value("length").toInteger(0);
    if (requested <= 0) {
        reply(ResultError("\"length\" must be at least 1."));
        return;
    }
    const quint64 limit = quint64(commonSettings().mcpMemoryReadLimit());
    const quint64 length = std::min(quint64(requested), limit);
    if (*address > std::numeric_limits<quint64>::max() - (length - 1)) {
        reply(ResultError("The range runs past the end of the address space."));
        return;
    }

    const Result<QString> grant = mcpMemoryAccessGrant(engine->runParameters(), *address, length);
    if (!grant) {
        reply(ResultError(grant.error()));
        return;
    }

    const QString encoding = args.value("encoding").toString("hex");
    const QPointer<McpSessionState> session = *state;
    const int stopId = session->stopId();
    auto answered = std::make_shared<bool>(false);
    QTimer::singleShot(timeoutArgument(args, defaultMemoryTimeoutMs), [answered, reply] {
        if (std::exchange(*answered, true))
            return;
        reply(ResultError("The debugger did not answer the memory read in time."));
    });
    engine->readMemory(*address, length,
                       [=](const MemoryReadResult &result) {
        if (std::exchange(*answered, true))
            return;
        if (!session || session->stopId() != stopId || !session->isStopped()) {
            reply(ResultError("The target resumed before the memory read completed."));
            return;
        }
        QJsonArray segments;
        quint64 offset = 0;
        quint64 unreadableBytes = 0;
        for (const MemoryReadResult::Unreadable &range : result.unreadable) {
            if (range.offset > offset)
                segments.append(memorySegment(result, offset, range.offset - offset, encoding, {}));
            segments.append(memorySegment(result, range.offset, range.length, encoding,
                                          range.reason));
            unreadableBytes += range.length;
            offset = range.offset + range.length;
        }
        if (offset < length)
            segments.append(memorySegment(result, offset, length - offset, encoding, {}));

        QJsonObject answer{{"address", QString("0x%1").arg(*address, 0, 16)},
                           {"requested_length", requested},
                           {"length", qint64(length)},
                           {"bytes_read", qint64(length - unreadableBytes)},
                           {"truncated", quint64(requested) > length},
                           {"encoding", encoding},
                           {"access", *grant},
                           {"segments", segments},
                           {"context", session->context()}};
        if (quint64(requested) > length) {
            answer["next_address"] = QString("0x%1").arg(*address + length, 0, 16);
            answer["note"] = QString("At most %1 bytes are read at once (Preferences > Debugger "
                                     "> General > MCP Server). Read the rest from "
                                     "next_address.").arg(limit);
        }
        reply(answer);
    });
}

// Capabilities

static QString startModeName(DebuggerStartMode mode)
{
    switch (mode) {
    case StartInternal:          return "start_project";
    case StartExternal:          return "start_executable";
    case AttachToLocalProcess:   return "attach_to_local_process";
    case AttachToCrashedProcess: return "attach_to_crashed_process";
    case AttachToCore:           return "core_file";
    case AttachToRemoteServer:   return "attach_to_remote_server";
    case AttachToRemoteProcess:  return "attach_to_remote_process";
    case AttachToQmlServer:      return "attach_to_qml_server";
    case StartRemoteProcess:     return "start_remote_process";
    case AttachToIosDevice:      return "attach_to_ios_device";
    default:                     return "unknown";
    }
}

static QString connectionType(const DebuggerRunParameters &rp)
{
    switch (rp.startMode()) {
    case StartInternal:
    case StartExternal:
        return rp.remoteChannel().isEmpty() ? QString("local") : QString("remote_server");
    case AttachToLocalProcess:
    case AttachToCrashedProcess:
        return "local";
    case AttachToCore:
        return "core_file";
    case AttachToRemoteServer:
        return "remote_server";
    case AttachToRemoteProcess:
    case StartRemoteProcess:
        return "remote_device";
    case AttachToQmlServer:
        return "qml_debug_server";
    case AttachToIosDevice:
        return "ios_device";
    default:
        return "unknown";
    }
}

static QString sessionType(DebuggerStartMode mode)
{
    switch (mode) {
    case StartInternal:
    case StartExternal:
    case StartRemoteProcess:
        return "launch";
    case AttachToLocalProcess:
    case AttachToRemoteServer:
    case AttachToRemoteProcess:
    case AttachToQmlServer:
    case AttachToIosDevice:
        return "attach";
    case AttachToCrashedProcess:
    case AttachToCore:
        return "post_mortem";
    default:
        return "unknown";
    }
}

static QJsonObject sessionEnvironment(const DebuggerEngine *engine)
{
    const DebuggerRunParameters &rp = engine->runParameters();
    const ProjectExplorer::Abi abi = rp.toolChainAbi();
    const auto orUnknown = [](const QString &value) {
        return value.isEmpty() ? QString("unknown") : value;
    };
    QJsonObject env{
        {"debugger", orUnknown(engine->debuggerName())},
        {"engine", orUnknown(engine->objectName())},
        {"debugger_executable", orUnknown(rp.debugger().command.executable().toUserOutput())},
        {"target_architecture",
         abi.architecture() == ProjectExplorer::Abi::UnknownArchitecture
             ? QString("unknown") : ProjectExplorer::Abi::toString(abi.architecture())},
        {"target_os", abi.os() == ProjectExplorer::Abi::UnknownOS
                          ? QString("unknown") : ProjectExplorer::Abi::toString(abi.os())},
        {"target_word_width", abi.wordWidth() == 0 ? QJsonValue("unknown")
                                                    : QJsonValue(int(abi.wordWidth()))},
        {"start_mode", startModeName(rp.startMode())},
        {"connection_type", connectionType(rp)},
        {"session_type", sessionType(rp.startMode())},
        {"languages", QJsonArray::fromStringList(
             QStringList{rp.isCppDebugging() ? QString("cpp") : QString(),
                         rp.isQmlDebugging() ? QString("qml") : QString()}
                 .filter(QRegularExpression(".")))},
    };
    if (!rp.remoteChannel().isEmpty())
        env["remote_channel"] = rp.remoteChannel();
    return env;
}

enum class BackendSupport { Supported, Unsupported, Unknown };
enum class Permission { Granted, Restricted, Denied };

static QString toString(BackendSupport support)
{
    switch (support) {
    case BackendSupport::Supported:   return "supported";
    case BackendSupport::Unsupported: return "unsupported";
    case BackendSupport::Unknown:     return "unknown";
    }
    return {};
}

static QString toString(Permission permission)
{
    switch (permission) {
    case Permission::Granted:    return "granted";
    case Permission::Restricted: return "restricted";
    case Permission::Denied:     return "denied";
    }
    return {};
}

class CapabilityCheck
{
public:
    QString name;
    QStringList tools;
    BackendSupport backendSupport;
    QString neededState; // The session state the tools need.
    bool stateOk = true;
    std::optional<Permission> permission; // Where one applies.
    QString permissionReason;
    QString note;
};

static QJsonObject capabilityToJson(const CapabilityCheck &c, bool hasSession,
                                    const QString &stateName)
{
    bool exposed = !c.tools.isEmpty();
    for (const QString &tool : c.tools)
        exposed = exposed && Mcp::ToolRegistry::isToolEnabled(tool);

    QString status;
    QString reason;
    if (!exposed) {
        status = "not_exposed";
        reason = "A tool for this is not offered by the MCP server, or turned off in its settings.";
    } else if (!hasSession) {
        status = "invalid_state";
        reason = "No debug session is active.";
    } else if (c.backendSupport == BackendSupport::Unsupported) {
        status = "unsupported";
        reason = "The debugger of this session does not support this.";
    } else if (c.permission == Permission::Denied) {
        status = "permission_denied";
        reason = c.permissionReason;
    } else if (!c.stateOk) {
        status = "invalid_state";
        reason = QString("Needs a %1 session; the session is %2.").arg(c.neededState, stateName);
    } else if (c.backendSupport == BackendSupport::Unknown) {
        status = "unknown";
        reason = "Whether the debugger of this session supports this is not known until it "
                 "is tried.";
    } else {
        status = "available";
    }

    QJsonObject obj{{"name", c.name},
                    {"tools", QJsonArray::fromStringList(c.tools)},
                    {"exposed", exposed},
                    {"backend_support", toString(c.backendSupport)},
                    {"requires", c.neededState},
                    {"status", status}};
    if (!reason.isEmpty())
        obj["reason"] = reason;
    if (c.permission) {
        obj["permission"] = toString(*c.permission);
        if (!c.permissionReason.isEmpty())
            obj["permission_reason"] = c.permissionReason;
    }
    if (!c.note.isEmpty())
        obj["note"] = c.note;
    return obj;
}

static void getCapabilities(const QJsonObject &, const McpReply &reply)
{
    const QPointer<DebuggerEngine> engine = EngineManager::currentEngine();
    const bool hasSession = !engine.isNull();
    const DebuggerState state = hasSession ? engine->state() : DebuggerNotReady;
    const bool paused = state == InferiorStopOk;
    const bool running = state == InferiorRunOk;
    const auto support = [&](unsigned cap) {
        if (!hasSession)
            return BackendSupport::Unknown;
        return engine->hasCapability(cap) ? BackendSupport::Supported
                                          : BackendSupport::Unsupported;
    };

    QList<CapabilityCheck> checks;
    checks.append({"execution_control",
                   {"debugger_step_over", "debugger_step_in", "debugger_step_out",
                    "debugger_continue", "debugger_interrupt", "debugger_wait_for_operation"},
                   BackendSupport::Supported, "paused or running", paused || running, {}, {},
                   "Stepping and continuing need a paused session, interrupting a running one."});
    checks.append({"run_to_line", {"debugger_run_to_line"}, support(RunToLineCapability),
                   "paused", paused});
    checks.append({"call_stack", {"debugger_get_call_stack", "debugger_select_frame"},
                   BackendSupport::Supported, "paused", paused});

    BackendSupport threadSupport = BackendSupport::Unknown;
    QString threadNote = "Lists the threads the debugger reports. Whether those include RTOS "
                         "tasks depends on the debugger and its server, not on Qt Creator.";
    if (hasSession && engine->threadsHandler()->rootItem()->childCount() > 0) {
        threadSupport = BackendSupport::Supported;
    } else if (hasSession) {
        threadNote += " The debugger has not reported any thread in this session yet.";
    }
    checks.append({"threads", {"debugger_get_threads", "debugger_select_thread"},
                   threadSupport, "paused", paused, {}, {}, threadNote});
    checks.append({"thread_stacks", {"debugger_get_thread_stacks"}, threadSupport, "paused",
                   paused, {}, {},
                   "Reads the threads one after another while the target stays stopped, not "
                   "as an atomic snapshot."});
    checks.append({"variables",
                   {"debugger_get_variables", "debugger_get_variable", "debugger_set_variable"},
                   BackendSupport::Supported, "paused", paused});
    checks.append({"expressions",
                   {"debugger_evaluate_expression", "debugger_add_watch_expression",
                    "debugger_remove_watch_expression"},
                   support(AddWatcherCapability), "paused", paused});
    checks.append({"registers", {"debugger_get_registers"}, support(RegisterCapability),
                   "paused", paused});

    CapabilityCheck memory{"memory_read", {"debugger_read_memory"},
                           hasSession ? (engine->canReadMemory() ? BackendSupport::Supported
                                                                 : BackendSupport::Unsupported)
                                      : BackendSupport::Unknown,
                           "paused", paused};
    const CommonSettings &s = commonSettings();
    if (s.mcpMemoryAccess() == CommonSettings::McpMemoryNever) {
        memory.permission = Permission::Denied;
        memory.permissionReason = "Reading memory over MCP is turned off in Preferences > "
                                  "Debugger > General > MCP Server.";
    } else if (s.mcpMemoryAccess() == CommonSettings::McpMemoryAnywhere) {
        memory.permission = Permission::Granted;
        memory.permissionReason = "Any address may be read, including memory-mapped I/O.";
    } else if (!hasSession || debugsProcessMemory(engine->runParameters())) {
        memory.permission = Permission::Granted;
        memory.permissionReason = "Process memory may be read.";
    } else if (s.mcpMemoryRanges().trimmed().isEmpty()) {
        memory.permission = Permission::Denied;
        memory.permissionReason = "This session debugs a bare metal target, or one of unknown "
                                  "kind, and no readable ranges are configured in Preferences "
                                  "> Debugger > General > MCP Server.";
    } else {
        memory.permission = Permission::Restricted;
        memory.permissionReason = "Only these ranges may be read: " + s.mcpMemoryRanges();
    }
    memory.note = QString("At most %1 bytes per read.").arg(s.mcpMemoryReadLimit());
    checks.append(memory);

    checks.append({"breakpoints",
                   {"debugger_get_breakpoints", "debugger_add_breakpoint",
                    "debugger_delete_breakpoint"},
                   BackendSupport::Supported, "any", true});
    checks.append({"conditional_breakpoints", {"debugger_add_breakpoint"},
                   support(BreakConditionCapability), "any", true});
    checks.append({"watchpoints_by_address", {"debugger_add_breakpoint"},
                   support(WatchpointByAddressCapability), "any", true});
    checks.append({"watchpoints_by_expression", {"debugger_add_breakpoint"},
                   support(WatchpointByExpressionCapability), "any", true});
    checks.append({"reverse_stepping", {}, support(ReverseSteppingCapability), "paused",
                   paused, {}, {}, "Available in Qt Creator, not over MCP."});
    checks.append({"jump_to_line", {}, support(JumpToLineCapability), "paused", paused, {}, {},
                   "Available in Qt Creator, not over MCP."});

    const QString stateName = hasSession ? DebuggerEngine::stateName(state) : QString("none");
    QJsonArray capabilities;
    for (const CapabilityCheck &check : std::as_const(checks))
        capabilities.append(capabilityToJson(check, hasSession, stateName));

    QJsonObject result{{"has_session", hasSession},
                       {"state", stateName},
                       {"capabilities", capabilities}};
    if (hasSession) {
        result["environment"] = sessionEnvironment(engine);
        result["context"] = McpSessionState::forEngine(engine)->context();
    }
    reply(result);
}

void registerIntrospectionMcpTools()
{
    using namespace Mcp::Schema;

    const QJsonObject frameSchema{
        {"type", "object"},
        {"required", QJsonArray{"level"}},
        {"properties", QJsonObject{
            {"level",    QJsonObject{{"type", "integer"}, {"description", "Frame index, 0 = innermost."}}},
            {"function", QJsonObject{{"type", "string"}}},
            {"file",     QJsonObject{{"type", "string"}}},
            {"line",     QJsonObject{{"type", "integer"}}},
            {"address",  QJsonObject{{"type", "string"}, {"description", "Instruction address, such as \"0x1234abcd\"."}}},
            {"module",   QJsonObject{{"type", "string"}}},
        }}};
    const QJsonObject threadSchema{
        {"type", "object"},
        {"required", QJsonArray{"id"}},
        {"properties", QJsonObject{
            {"id",                QJsonObject{{"type", "string"}}},
            {"name",              QJsonObject{{"type", "string"}}},
            {"state",             QJsonObject{{"type", "string"}}},
            {"target_id",         QJsonObject{{"type", "string"}}},
            {"frames",            QJsonObject{{"type", "array"}, {"items", frameSchema}}},
            {"frames_loaded",     QJsonObject{{"type", "integer"}, {"description", "How many frames the debugger had loaded for the thread."}}},
            {"truncated",         QJsonObject{{"type", "boolean"}, {"description", "The thread has more frames than returned."}}},
            {"truncation_reason", QJsonObject{{"type", "string"}}},
            {"error",             QJsonObject{{"type", "string"}, {"description", "Why no stack was collected for this thread."}}},
        }}};

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_thread_stacks")
            .title("Get the call stacks of several threads")
            .description(
                "Collects the call stacks of all threads, or of the given threads, of the paused "
                "debug session, for example to look for a deadlock. Selects each thread in turn "
                "and selects the original thread and frame again at the end. Does not resume the "
                "target. Reports per thread when a stack could not be collected, and reports when "
                "the target resumed during collection instead of mixing stacks from different "
                "stops. Covers the threads the debugger reports, which is not necessarily every "
                "CPU, process or RTOS task of the device.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("thread_ids",
                                 QJsonObject{{"type", "array"},
                                             {"items", QJsonObject{{"type", "string"}}},
                                             {"description", "Threads to collect, as returned by "
                                                             "debugger_get_threads. Default: all."}})
                    .addProperty("max_frames",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"maximum", maximalMaxFrames},
                                             {"default", defaultMaxFrames},
                                             {"description", "Frames per thread, innermost first."}})
                    .addProperty("max_threads",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"maximum", maximalMaxThreads},
                                             {"default", defaultMaxThreads},
                                             {"description", "Threads to collect at most. The "
                                                             "rest is listed in omitted_threads."}})
                    .addProperty("max_total_frames",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"maximum", maximalMaxTotalFrames},
                                             {"default", defaultMaxTotalFrames},
                                             {"description", "Frames over all threads at most."}})
                    .addProperty("fields",
                                 QJsonObject{{"type", "array"},
                                             {"items", QJsonObject{{"type", "string"},
                                                                   {"enum", QJsonArray::fromStringList(frameFields())}}},
                                             {"description", "Frame fields to return besides "
                                                             "level. Default: all."}})
                    .addProperty("timeout_ms", timeoutSchema(defaultThreadTimeoutMs)))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("threads", QJsonObject{{"type", "array"}, {"items", threadSchema}})
                    .addProperty("stop_id", QJsonObject{{"type", "integer"},
                                                        {"description", "The stop the collection started at."}})
                    .addProperty("same_stop", QJsonObject{{"type", "boolean"},
                                                          {"description", "All stacks come from that stop."}})
                    .addProperty("target_resumed", QJsonObject{{"type", "boolean"}})
                    .addProperty("selection_restored", QJsonObject{{"type", "boolean"}})
                    .addProperty("selection_problem", QJsonObject{{"type", "string"}})
                    .addProperty("omitted_threads", QJsonObject{{"type", "array"},
                                                                {"items", QJsonObject{{"type", "string"}}}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addProperty("context", contextSchema())
                    .addRequired("threads")
                    .addRequired("same_stop")
                    .addRequired("target_resumed")),
        collectThreadStacks);

    const QJsonObject registerSchema{
        {"type", "object"},
        {"required", QJsonArray{"name", "available"}},
        {"properties", QJsonObject{
            {"name",      QJsonObject{{"type", "string"}}},
            {"value",     QJsonObject{{"type", "string"},
                                      {"description", "All bits in hex, zero-padded to the register "
                                                      "width, such as \"0x000000016fdff2a0\". Absent "
                                                      "when the value is not available."}}},
            {"available", QJsonObject{{"type", "boolean"}}},
            {"reason",    QJsonObject{{"type", "string"}, {"description", "Why no value is available."}}},
            {"bit_width", QJsonObject{{"type", "integer"}}},
            {"kind",      QJsonObject{{"type", "string"},
                                      {"enum", QJsonArray{"integer", "float", "vector", "flags", "other", "unknown"}}}},
            {"type",      QJsonObject{{"type", "string"}, {"description", "The type the debugger reports."}}},
            {"groups",    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}},
        }}};

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_registers")
            .title("Get CPU registers")
            .description(
                "Returns the CPU registers of the selected thread and frame of the paused debug "
                "session, read freshly from the debugger. Read-only. Values are given in hex with "
                "all bits; a register without a value is reported as not available rather than "
                "as zero. For a frame other than the innermost, values_for_frame_level says whether "
                "the debugger reconstructed the values for that frame or reports the innermost "
                "frame's. Returns an error if the debugger cannot access "
                "registers, if no session is active or if it is not paused.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("names",
                                 QJsonObject{{"type", "array"},
                                             {"items", QJsonObject{{"type", "string"}}},
                                             {"description", "Registers to return, such as "
                                                             "[\"pc\", \"sp\"]. Default: all."}})
                    .addProperty("group",
                                 QJsonObject{{"type", "string"},
                                             {"description", "Only registers in this group, as "
                                                             "listed in \"groups\" of an earlier "
                                                             "answer."}})
                    .addProperty("timeout_ms", timeoutSchema(defaultRegisterTimeoutMs)))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("registers", QJsonObject{{"type", "array"}, {"items", registerSchema}})
                    .addProperty("groups", QJsonObject{{"type", "array"},
                                                       {"items", QJsonObject{{"type", "string"}}},
                                                       {"description", "All register groups the debugger reports."}})
                    .addProperty("frame_level", QJsonObject{{"type", "integer"}})
                    .addProperty("live_cpu_values", QJsonObject{{"type", "boolean"},
                                                                {"description", "The values are the current CPU state, not reconstructed for an outer frame. Absent when that is not known."}})
                    .addProperty("values_for_frame_level", QJsonObject{{"type", "integer"},
                                                                       {"description", "The frame the values belong to, which can differ from frame_level. Absent when that is not known."}})
                    .addProperty("unknown_names", QJsonObject{{"type", "array"},
                                                              {"items", QJsonObject{{"type", "string"}}},
                                                              {"description", "Requested names the debugger does not report."}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addProperty("context", contextSchema())
                    .addRequired("registers")
                    .addRequired("context")),
        getRegisters);

    const QJsonObject segmentSchema{
        {"type", "object"},
        {"required", QJsonArray{"address", "offset", "length", "readable"}},
        {"properties", QJsonObject{
            {"address",  QJsonObject{{"type", "string"}}},
            {"offset",   QJsonObject{{"type", "integer"}, {"description", "From the start of the read."}}},
            {"length",   QJsonObject{{"type", "integer"}}},
            {"readable", QJsonObject{{"type", "boolean"}}},
            {"data",     QJsonObject{{"type", "string"}, {"description", "The bytes, when readable."}}},
            {"reason",   QJsonObject{{"type", "string"}, {"description", "Why the bytes could not be read."}}},
        }}};

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_read_memory")
            .title("Read target memory")
            .description(
                "Reads bytes of target memory from an address in the paused debug session. "
                "Read-only. Returns the range as segments, each either readable with its bytes "
                "or not readable with a reason; unreadable bytes are never returned as zeros. "
                "Reads at most the number of bytes configured in Preferences > Debugger > "
                "General > MCP Server and says so when it read less than asked. On bare metal "
                "targets and targets of unknown kind, where an address may be memory-mapped I/O, "
                "only ranges allowed there can be read.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .inputSchema(
                Tool::InputSchema{}
                    .addProperty("address",
                                 QJsonObject{{"type", QJsonArray{"string", "integer"}},
                                             {"description", "Start address, such as \"0x16fdff2a0\" "
                                                             "or a decimal number."}})
                    .addProperty("length",
                                 QJsonObject{{"type", "integer"},
                                             {"minimum", 1},
                                             {"description", "Number of bytes to read."}})
                    .addProperty("encoding",
                                 QJsonObject{{"type", "string"},
                                             {"enum", QJsonArray{"hex", "base64"}},
                                             {"default", "hex"}})
                    .addProperty("timeout_ms", timeoutSchema(defaultMemoryTimeoutMs))
                    .addRequired("address")
                    .addRequired("length"))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("address", QJsonObject{{"type", "string"}})
                    .addProperty("requested_length", QJsonObject{{"type", "integer"}})
                    .addProperty("length", QJsonObject{{"type", "integer"},
                                                       {"description", "Bytes covered by the segments."}})
                    .addProperty("bytes_read", QJsonObject{{"type", "integer"}})
                    .addProperty("truncated", QJsonObject{{"type", "boolean"},
                                                          {"description", "Fewer bytes were read than asked for, because of the size limit."}})
                    .addProperty("next_address", QJsonObject{{"type", "string"}})
                    .addProperty("encoding", QJsonObject{{"type", "string"}})
                    .addProperty("access", QJsonObject{{"type", "string"},
                                                       {"enum", QJsonArray{"process_memory", "allowed_range", "any_memory"}},
                                                       {"description", "Why the read was allowed."}})
                    .addProperty("segments", QJsonObject{{"type", "array"}, {"items", segmentSchema}})
                    .addProperty("note", QJsonObject{{"type", "string"}})
                    .addProperty("context", contextSchema())
                    .addRequired("address")
                    .addRequired("length")
                    .addRequired("segments")
                    .addRequired("truncated")),
        readMemory);

    const QJsonObject capabilitySchema{
        {"type", "object"},
        {"required", QJsonArray{"name", "tools", "exposed", "backend_support", "status"}},
        {"properties", QJsonObject{
            {"name",              QJsonObject{{"type", "string"}}},
            {"tools",             QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}},
            {"exposed",           QJsonObject{{"type", "boolean"}, {"description", "The tools are offered and turned on in the MCP server."}}},
            {"backend_support",   QJsonObject{{"type", "string"},
                                              {"enum", QJsonArray{toString(BackendSupport::Supported),
                                                                  toString(BackendSupport::Unsupported),
                                                                  toString(BackendSupport::Unknown)}}}},
            {"requires",          QJsonObject{{"type", "string"}, {"description", "The session state the tools need."}}},
            {"status",            QJsonObject{{"type", "string"},
                                              {"enum", QJsonArray{"available", "unsupported", "invalid_state",
                                                                  "permission_denied", "not_exposed", "unknown"}}}},
            {"reason",            QJsonObject{{"type", "string"}}},
            {"permission",        QJsonObject{{"type", "string"},
                                              {"enum", QJsonArray{toString(Permission::Granted),
                                                                  toString(Permission::Restricted),
                                                                  toString(Permission::Denied)}}}},
            {"permission_reason", QJsonObject{{"type", "string"}}},
            {"note",              QJsonObject{{"type", "string"}}},
        }}};

    registerAsyncMcpTool(
        Tool{}
            .name("debugger_get_capabilities")
            .title("Get debugger capabilities")
            .description(
                "Reports the debug session's environment (debugger, target architecture and "
                "OS, connection and session type; \"unknown\" where Qt Creator does not know) "
                "and, for each kind of operation, whether its tools are offered, whether the "
                "session's debugger supports it, whether the session is in a state for it, and "
                "whether it is permitted. Call this before choosing how to investigate.")
            .annotations(ToolAnnotations{}.readOnlyHint(true))
            .outputSchema(
                Tool::OutputSchema{}
                    .addProperty("has_session", QJsonObject{{"type", "boolean"}})
                    .addProperty("state", QJsonObject{{"type", "string"}})
                    .addProperty("environment", QJsonObject{{"type", "object"}})
                    .addProperty("capabilities", QJsonObject{{"type", "array"}, {"items", capabilitySchema}})
                    .addProperty("context", contextSchema())
                    .addRequired("has_session")
                    .addRequired("capabilities")),
        getCapabilities);
}

} // namespace Debugger::Internal
