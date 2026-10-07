// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "debuggerengine.h"
#include "mcpsessionstate.h"
#include "mcpsupport_p.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "threadshandler.h"

#include <mcp/server/toolregistry.h>

#include <utils/qtcassert.h>

#include <QJsonArray>
#include <QPointer>
#include <QSet>

#include <algorithm>

using namespace Utils;

namespace Debugger::Internal {

enum {
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
    const quint64 words[4] = {value.v.u128[0].lo, value.v.u128[0].hi,
                              value.v.u128[1].lo, value.v.u128[1].hi};
    const int bytes = size > 0 ? std::min(size, 32) : 32;
    QString hex;
    for (int i = bytes - 1; i >= 0; --i) {
        const uint byte = (words[i / 8] >> (8 * (i % 8))) & 0xff;
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
}

} // namespace Debugger::Internal
