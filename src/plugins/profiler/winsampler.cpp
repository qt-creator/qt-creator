// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "winsampler.h"

// The Qt Creator PCH undefines these Windows macros; restore them before the
// Windows headers below are pulled in.
#ifdef QTCREATOR_PCH_H
#define CALLBACK __stdcall
#define IN
#define OUT
#endif
#include <qt_windows.h>

#include "sampletrace.h"
#include "winsymbolicator.h"

#include "profilertr.h"

#include <utils/processinfo.h>
#include <utils/synchronizedvalue.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QScopeGuard>
#include <QStandardPaths>

#include <evntcons.h>
#include <evntrace.h>
#include <psapi.h>
#include <winternl.h>

#include <algorithm>
#include <atomic>
#include <thread>
#include <utility>
#include <vector>

using namespace Profiler;
using namespace Utils;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {
namespace {

// ETW GUIDs, with the values evntrace.h defines (the SDK only provides them
// via DEFINE_GUID, which needs INITGUID in exactly one TU, so they are
// spelled out here). SystemTraceControlGuid identifies the NT Kernel Logger
// session for StartTrace/ControlTrace; it is never an event's ProviderId.
// Classic kernel events arrive under their event-class GUID instead:
// StackWalkGuid carries the sampled call stacks this backend consumes,
// ImageLoadGuid signals that the target's module list changed, and
// PerfInfoGuid names the event class that stack walking is switched on for.
// See also macsampler.cpp (macOS) and perfsampler.cpp (Linux), which are the
// other two sampler backends.
constexpr GUID SystemTraceControlGuid = { // {9E814AAD-3204-11D2-9A82-006008A86939}
    0x9E814AAD, 0x3204, 0x11D2,
    {0x9A, 0x82, 0x00, 0x60, 0x08, 0xA8, 0x69, 0x39}
};

constexpr GUID StackWalkGuid = {
    0xDEF2FE46, 0x7BD6, 0x4B80,
    {0xBD, 0x94, 0xF5, 0x7F, 0xE2, 0x0D, 0x0C, 0xE3}
};

constexpr GUID PerfInfoGuid = {
    0xCE1DBFB4, 0x137E, 0x4DA6,
    {0x87, 0xB0, 0x3F, 0x59, 0xAA, 0x10, 0x2C, 0xBC}
};

constexpr GUID ImageLoadGuid = {
    0x2CB15D1D, 0x5FC1, 0x11D2,
    {0xAB, 0xE1, 0x00, 0xA0, 0xC9, 0x11, 0xF5, 0x18}
};

// SampledProfile event opcode (V2).
constexpr UCHAR kSampledProfileOpcode = 46;

// StackWalk event opcode (V2).
constexpr UCHAR kStackWalkOpcode = 32;

// Image_Load "Load" opcode (0x0A). Not to be confused with the rundown
// opcodes (DCStart/DCEnd) or Unload, which share the ImageLoadGuid.
constexpr UCHAR kImageLoadOpcode = EVENT_TRACE_TYPE_LOAD;

// Maximum stack depth captured by ETW.
constexpr int kMaxEtwStackDepth = 192;

// StackWalkEvent payload layout: timestamp, PID, TID, then stack array.
struct StackWalkHeader
{
    quint64 eventTimeStamp;
    quint32 stackProcess;
    quint32 stackThread;
};

static_assert(offsetof(StackWalkHeader, stackProcess) == 8);
static_assert(offsetof(StackWalkHeader, stackThread) == 12);

// Elevate SE_SYSTEM_PROFILE_NAME privilege for kernel profiling.
bool elevateProfilePrivilege()
{
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        qCWarning(etwLog, "OpenProcessToken failed (0x%lx).", GetLastError());
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValueW(nullptr, L"SeSystemProfilePrivilege", &tp.Privileges[0].Luid)) {
        qCWarning(etwLog, "LookupPrivilegeValue failed (0x%lx).", GetLastError());
        CloseHandle(hToken);
        return false;
    }

    // AdjustTokenPrivileges reports partial success through GetLastError, so the
    // return value alone does not mean the privilege was actually enabled.
    const bool adjusted = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    const DWORD lastError = GetLastError();
    const bool success = adjusted && lastError == ERROR_SUCCESS;
    qCDebug(etwLog, "elevateProfilePrivilege %s (error=%lu).",
            success ? "succeeded" : "failed", lastError);
    CloseHandle(hToken);
    return success;
}

// Find a running process by executable basename.
Result<DWORD> findProcessByName(const QString &name)
{
    const auto infos = ProcessInfo::processInfoList();
    if (!infos)
        return ResultError(Tr::tr("Cannot enumerate running processes."));

    for (const auto &info : *infos) {
        if (QFileInfo(info.executable).fileName() == name)
            return static_cast<DWORD>(info.processId);
    }
    return ResultError(Tr::tr("No running process named \"%1\" was found.").arg(name));
}

// The part of the trace the ETW callback builds up. ProcessTrace delivers all
// events sequentially on the capture thread; the lock guards against the
// controlling thread, which reads the sample count while stopping and takes
// the buffer over once the capture thread has joined. The labels the samples
// index are filled in separately by LiveLabeler, which has its own lock — so
// keeping the two apart until the end avoids one object guarded by two mutexes.
struct CaptureBuffer
{
    // One StackWalk event's worth of frames. A profile hit in user mode is one
    // event; a hit in kernel mode arrives as *two* — the kernel stack at
    // interrupt time and the user stack once the thread returns to user mode —
    // sharing (rawTs, tid). recordSampleTrace() stitches those pairs back into
    // one sample; recording them separately would double-count the thread in
    // the CPU usage model and tear every kernel-mode hit into two half stacks.
    struct Fragment
    {
        SampleTraceData::ThreadSample sample;
        quint64 rawTs = 0;   // event timestamp in raw QPC ticks
        bool kernel = false; // innermost frame is a kernel-mode address
    };
    QList<Fragment> fragments;
    QHash<quint64, QString> threadNames; // tid -> name (entries may be empty)
};

// Shared state for ETW capture, accessed from the event callback threads.
struct EtwCaptureContext
{
    // `process` must stay open for the lifetime of the context: the symbolicator
    // and the labeler read the target's module list through it.
    EtwCaptureContext(HANDLE process, DWORD pid, int interval)
        : targetPid(pid)
        , intervalUs(interval)
        , symbolicator(process)
        , labeler(symbolicator, data)
    {
        data.pid = pid;
        LARGE_INTEGER frequency;
        if (QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0)
            qpcTicksPerSecond = static_cast<quint64>(frequency.QuadPart);
        // The context is created when the capture goes live, and the session
        // clock is QPC (see startEtwSession), so QPC "now" is the trace's zero.
        LARGE_INTEGER now;
        if (QueryPerformanceCounter(&now) && now.QuadPart > 0)
            startEventTimestamp.store(static_cast<quint64>(now.QuadPart),
                                      std::memory_order_relaxed);
    }

    quint64 targetPid = 0;
    int intervalUs = 200; // Sampling interval in microseconds.

    // Declaration order matters: `labeler` binds references to the two above it.
    SampleTraceData data;
    Symbolicator symbolicator;
    LiveLabeler labeler;

    SynchronizedValue<CaptureBuffer> buffer;

    // Approximate size of everything in `buffer`. Only ever compared against a
    // soft cap, so it is counted lock-free rather than under the buffer's lock.
    std::atomic<size_t> sampleBytes{0};
    static constexpr size_t kMaxSampleBytes = 512ull * 1024 * 1024;

    // Debug counters (atomic for lock-free access from callback thread).
    std::atomic<size_t> eventsReceived{0};
    std::atomic<size_t> stackWalkEvents{0};

    // One-shot flag to warn once when the sample memory budget is exceeded.
    std::atomic<bool> sampleBudgetExceeded{false};

    // Session and consumer handles. Both are established by startEtwSession()
    // on the calling thread before the capture thread starts, so the capture
    // thread and the stopping thread only ever read them. Plain TRACEHANDLE,
    // not the CONTROLTRACE_ID/PROCESSTRACE_HANDLE aliases: those are typedefs
    // of it that only exist since Windows SDK 10.0.26100 (and not in MinGW).
    TRACEHANDLE sessionHandle = 0;
    TRACEHANDLE consumerHandle = INVALID_PROCESSTRACE_HANDLE;
    std::vector<UCHAR> sessionProps; // Must outlive the session.

    // Set when stopping: makes the buffer callback return FALSE, which ends
    // ProcessTrace at the next buffer even if ControlTrace(STOP) failed, so
    // the capture thread can always be joined.
    std::atomic<bool> stopRequested{false};

    // One-shot flag so event loss is warned about once, not per buffer.
    std::atomic<bool> eventLossReported{false};

    // Tick rate of the session clock (QPC, see startEtwSession). 10 MHz on most
    // x64 machines, but e.g. ARM64 typically differs, so it must be queried
    // rather than assumed.
    quint64 qpcTicksPerSecond = 10'000'000;

    // The sampled-profile interval is machine-global state; the value found at
    // startup, remembered here, is restored when the recording ends. 0 = the
    // query failed, nothing to restore.
    ULONG previousProfileInterval = 0;

    // Timestamp baseline (QPC ticks since system boot): the trace's zero
    // point, captured in the constructor when the capture goes live. Anchoring
    // it there rather than on the first sample keeps the trace aligned with
    // RecordingSession::startedMonotonicUs, which a combined recording uses to
    // correlate this trace with the QML one — a target that idles before its
    // first sample must not shift the timeline. 0 only if
    // QueryPerformanceCounter failed; the first sample then anchors it.
    std::atomic<quint64> startEventTimestamp{0};

    // Event-driven module refresh flag: set when an ImageLoad event arrives for
    // the target PID, cleared on the next stack-walk callback that performs a
    // refresh.
    std::atomic<bool> moduleRefreshPending{false};
};

// Resolve a thread name for a given TID using GetThreadDescription (Windows 10
// 1607+), falling back to the module name derived from the thread's start
// address via NtQueryInformationThread + VirtualQueryEx.
QString resolveThreadName(DWORD tid)
{
    // Spelled out rather than decltype(&GetThreadDescription): MinGW headers
    // do not declare GetThreadDescription at all.
    using GetThreadDescriptionFn = HRESULT (WINAPI *)(HANDLE, PWSTR *);
    static auto getThreadDesc = []() -> GetThreadDescriptionFn {
        HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
        return hKernel32
                   ? reinterpret_cast<GetThreadDescriptionFn>(
                         GetProcAddress(hKernel32, "GetThreadDescription"))
                   : nullptr;
    }();

    using NtQueryInformationThreadFn = NTSTATUS(WINAPI *)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
    static auto ntQuery = []() -> NtQueryInformationThreadFn {
        return reinterpret_cast<NtQueryInformationThreadFn>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationThread"));
    }();

    // First try: GetThreadDescription (Windows 10 1607+, hence the lookup
    // through GetProcAddress above).
    HANDLE hThread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!hThread)
        hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, tid);
    if (!hThread)
        return {};

    QString name;
    if (getThreadDesc) {
        PWSTR desc = nullptr;
        if (SUCCEEDED(getThreadDesc(hThread, &desc)) && desc) {
            name = QString::fromWCharArray(desc);
            LocalFree(desc);
        }
    }
    if (!name.isEmpty()) {
        CloseHandle(hThread);
        return name;
    }

    // Second try: get the thread's start address and attribute it to a module.
    if (ntQuery) {
        void *startAddr = nullptr;
        ULONG retLen = 0;
        // ThreadQuerySetWin32StartAddress, which winternl.h does not declare.
        constexpr auto kThreadQuerySetWin32StartAddress = static_cast<THREADINFOCLASS>(9);
        const NTSTATUS status = ntQuery(hThread, kThreadQuerySetWin32StartAddress,
                                        reinterpret_cast<PVOID>(&startAddr), sizeof(startAddr),
                                        &retLen);
        if (status == 0) {
            const auto hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                                              GetProcessIdOfThread(hThread));
            if (hProcess) {
                MEMORY_BASIC_INFORMATION vmemInfo{};
                if (VirtualQueryEx(hProcess, startAddr, &vmemInfo, sizeof(vmemInfo)) == sizeof(vmemInfo)
                    && vmemInfo.Type == MEM_IMAGE) {
                    HMODULE mod = reinterpret_cast<HMODULE>(vmemInfo.AllocationBase);
                    wchar_t moduleName[MAX_PATH] = {};
                    if (GetModuleFileNameExW(hProcess, mod, moduleName, ARRAYSIZE(moduleName))) {
                        name = QFileInfo(QString::fromWCharArray(moduleName)).fileName();
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }

    CloseHandle(hThread);
    return name;
}

// Process a single StackWalkEvent, extracting frames and symbolizing.
void processStackWalk(const EVENT_RECORD &record, EtwCaptureContext &ctx)
{
    const auto *rawData = reinterpret_cast<const UCHAR *>(record.UserData);
    const ULONG dataLen = record.UserDataLength;

    if (dataLen < sizeof(StackWalkHeader))
        return;

    const auto *header = reinterpret_cast<const StackWalkHeader *>(rawData);

    // Filter by target PID.
    if (header->stackProcess != static_cast<quint32>(ctx.targetPid))
        return;

    // Extract stack frames: bytes after header, each frame is 8 bytes.
    const ULONG stackOffset = sizeof(StackWalkHeader);
    if (dataLen <= stackOffset)
        return;

    const ULONG stackBytes = dataLen - stackOffset;
    const int frameCount = std::min(static_cast<int>(stackBytes / sizeof(quint64)), kMaxEtwStackDepth);

    const auto *frames = reinterpret_cast<const quint64 *>(rawData + stackOffset);

    // Use the ETW event timestamp (QPC ticks) instead of wall-clock time. QPC
    // is hardware-synchronized across CPUs, so samples from different CPUs
    // that occur simultaneously share the same timestamp — this is what the
    // CpuUsageModel needs to compute m_maxRunning.
    const quint64 tid = header->stackThread;

    // The baseline was taken when the capture went live (see the context
    // constructor); fall back to the first sample only if QPC failed there.
    // No race: all events arrive on the one ProcessTrace thread.
    quint64 baseline = ctx.startEventTimestamp.load(std::memory_order_relaxed);
    if (baseline == 0) {
        baseline = header->eventTimeStamp;
        ctx.startEventTimestamp.store(baseline, std::memory_order_relaxed);
    }

    // An event captured in the small window before the baseline was taken
    // clamps to 0 instead of letting the subtraction wrap. The tick rate is
    // whatever QueryPerformanceFrequency reports — commonly 10 MHz, but not
    // universally (e.g. ARM64), so no hard-coded divisor.
    const quint64 tsUsRelative = header->eventTimeStamp > baseline
        ? (header->eventTimeStamp - baseline) * 1'000'000 / ctx.qpcTicksPerSecond
        : 0;

    // Resolve thread name lazily on first encounter: try GetThreadDescription
    // first, then fall back to the module name from the start address. Resolving
    // it before taking the lock keeps OpenThread/VirtualQueryEx out of the
    // critical section the controlling thread also enters.
    const bool nameKnown =
        ctx.buffer.get([tid](const CaptureBuffer &b) { return b.threadNames.contains(tid); });
    if (!nameKnown) {
        const QString threadName = resolveThreadName(static_cast<DWORD>(tid));
        ctx.buffer.write([tid, &threadName](CaptureBuffer &b) {
            if (!b.threadNames.contains(tid))
                b.threadNames.insert(tid, threadName);
        });
    }

    // ETW stack is innermost-first (frame[0] = interrupted instruction).
    // SampleTraceData expects root-first (outermost call first), so reverse.
    QList<int> labelIds;
    labelIds.reserve(frameCount);

    // Event-driven module refresh: only re-enumerate when an ImageLoad event
    // has arrived for the target PID, clearing the flag to avoid redundant
    // EnumProcessModulesEx calls at kHz rates.
    if (ctx.moduleRefreshPending.exchange(false, std::memory_order_relaxed))
        ctx.labeler.refreshImages();
    for (int i = frameCount - 1; i >= 0; --i) {
        const quint64 addr = frames[i];
        if (addr == 0)
            continue;
        labelIds.append(ctx.labeler.labelIdFor(addr));
    }

    if (labelIds.isEmpty())
        return;

    // A stack walked in kernel mode is only the kernel half of the sample; the
    // matching user half arrives as a separate event (see CaptureBuffer). The
    // innermost frame tells the halves apart: kernel-mode addresses live in
    // the upper half of the canonical address space on both x64 and ARM64.
    bool kernelFragment = false;
    for (int i = 0; i < frameCount; ++i) {
        if (frames[i] != 0) {
            kernelFragment = (frames[i] >> 63) != 0;
            break;
        }
    }

    CaptureBuffer::Fragment fragment;
    fragment.rawTs = header->eventTimeStamp;
    fragment.kernel = kernelFragment;
    fragment.sample.tsUs = tsUsRelative;
    fragment.sample.tid = tid;
    fragment.sample.running = true; // SampledProfile events are on-CPU samples.
    fragment.sample.frames = std::move(labelIds);

    ctx.sampleBytes.fetch_add(sizeof(CaptureBuffer::Fragment)
                                  + size_t(fragment.sample.frames.size()) * sizeof(int),
                              std::memory_order_relaxed);
    ctx.buffer.write(
        [&fragment](CaptureBuffer &b) { b.fragments.append(std::move(fragment)); });
}

// ETW event record callback, invoked on the ProcessTrace worker thread.
void WINAPI onEventRecord(_In_ PEVENT_RECORD record)
{
    EtwCaptureContext *ctx = reinterpret_cast<EtwCaptureContext *>(record->UserContext);
    if (!ctx)
        return;

    ctx->eventsReceived.fetch_add(1, std::memory_order_relaxed);

    const GUID &provider = record->EventHeader.ProviderId;
    const UCHAR opcode = record->EventHeader.EventDescriptor.Opcode;

    // An image loaded into the target invalidates the labeler's module list.
    if (IsEqualGUID(provider, ImageLoadGuid) && opcode == kImageLoadOpcode) {
        // Image_Load payload: ImageBase and ImageSize (pointer-sized, using the
        // logging kernel's pointer size), then ProcessId.
        const size_t ptrSize =
            (record->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) ? 4 : 8;
        const size_t pidOffset = 2 * ptrSize;
        if (record->UserData && record->UserDataLength >= pidOffset + sizeof(quint32)) {
            const auto *data = static_cast<const UCHAR *>(record->UserData);
            quint32 pid = 0;
            memcpy(&pid, data + pidOffset, sizeof(pid));
            if (pid == static_cast<quint32>(ctx->targetPid))
                ctx->moduleRefreshPending.store(true, std::memory_order_relaxed);
        }
        return;
    }

    if (!IsEqualGUID(provider, StackWalkGuid) || opcode != kStackWalkOpcode)
        return;

    ctx->stackWalkEvents.fetch_add(1, std::memory_order_relaxed);

    if (ctx->sampleBytes.load(std::memory_order_relaxed) >= EtwCaptureContext::kMaxSampleBytes) {
        if (!ctx->sampleBudgetExceeded.exchange(true, std::memory_order_relaxed))
            qCWarning(etwLog, "Sample memory budget (%zu MiB) exceeded, dropping further "
                              "samples.", EtwCaptureContext::kMaxSampleBytes / (1024 * 1024));
        return;
    }

    processStackWalk(*record, *ctx);
}

// ETW buffer callback: reports event loss (once) and ends event processing on
// a stop request.
ULONG WINAPI onBufferComplete(_Inout_ PEVENT_TRACE_LOGFILEA trace)
{
    auto *ctx = reinterpret_cast<EtwCaptureContext *>(trace->Context);
    if (!ctx)
        return TRUE;

    if (trace->EventsLost > 0
        && !ctx->eventLossReported.exchange(true, std::memory_order_relaxed)) {
        qCWarning(etwLog, "%lu events lost.", trace->EventsLost);
    }

    // FALSE makes ProcessTrace return at the next buffer boundary — the safety
    // net that keeps the capture thread joinable even when ControlTrace(STOP)
    // failed and the session is still producing buffers.
    return ctx->stopRequested.load(std::memory_order_relaxed) ? FALSE : TRUE;
}

// ControlTrace overwrites the properties buffer it is handed with the session's
// own settings, so it never gets the template buffer that StartTrace and the
// stop path depend on — only a scratch copy of it.
ULONG controlTrace(TRACEHANDLE sessionHandle, const std::vector<UCHAR> &sessionProps,
                   ULONG controlCode, ULONG enableFlags = 0)
{
    std::vector<UCHAR> scratch = sessionProps;
    auto *props = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(scratch.data());
    props->EnableFlags = enableFlags;
    CHAR *loggerName = reinterpret_cast<CHAR *>(scratch.data()) + props->LoggerNameOffset;
    return ControlTraceA(sessionHandle, loggerName, props, controlCode);
}

// Start the ETW session and attach a real-time consumer to it. Connects to the
// singleton NT Kernel Logger — the only session that generates SampledProfile
// events (private kernel sessions do not).
//
// Runs on the caller's thread, before the capture thread is started, so that
// ctx.sessionHandle and ctx.sessionProps are fully published before either the
// capture thread or stopEtwSession() can look at them. Starting the session
// from the capture thread would race with a stop request arriving first, which
// would leave the session running and ProcessTrace blocked forever.
//
// This is the Windows-specific backend; see perfsampler.cpp for the Linux
// perf-based alternative and macsampler.cpp for the macOS call-stack sampler.
Result<> startEtwSession(EtwCaptureContext &ctx)
{
    // --- Prepare properties for NT Kernel Logger ---
    const QByteArray nameBytes = QString(KERNEL_LOGGER_NAMEA).toUtf8();
    const auto propsSize = static_cast<ULONG>(sizeof(EVENT_TRACE_PROPERTIES) + nameBytes.size() + 1);
    ctx.sessionProps.assign(propsSize, 0);

    auto *props = reinterpret_cast<EVENT_TRACE_PROPERTIES *>(ctx.sessionProps.data());
    props->Wnode.BufferSize = propsSize;
    // Use QPC as the clock source: it is hardware-synchronized across CPUs, so
    // simultaneous samples from different cores carry identical timestamps.
    // Event timestamps are raw QPC ticks (see ctx.qpcTicksPerSecond).
    props->Wnode.ClientContext = 1;
    props->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    props->Wnode.Guid = SystemTraceControlGuid;
    props->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    props->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    props->BufferSize = 1024; // KB
    props->MinimumBuffers = 16;
    props->MaximumBuffers = 32;
    memcpy(ctx.sessionProps.data() + props->LoggerNameOffset, nameBytes.constData(),
           nameBytes.size() + 1);

    CHAR *loggerName = reinterpret_cast<CHAR *>(ctx.sessionProps.data()) + props->LoggerNameOffset;

    // --- Stop any existing NT Kernel Logger session (may be from another profiler) ---
    {
        const ULONG st = controlTrace(0, ctx.sessionProps, EVENT_TRACE_CONTROL_STOP);
        if (st == ERROR_SUCCESS) {
            // The kernel logger is a singleton, so whoever was using it (WPR,
            // xperf, ...) just lost their trace. At least leave a hint.
            qCWarning(etwLog, "An NT Kernel Logger session was already running; "
                              "it was stopped to record this trace.");
        } else {
            // Non-success is expected when no session was running.
            qCDebug(etwLog, "Existing kernel logger: stop returned 0x%lx.", st);
        }
    }

    // --- Set global CPU sampling interval ---
    {
        // Remember the machine-global interval currently in effect, so
        // recordSampleTrace() can restore it when the recording ends.
        TRACE_PROFILE_INTERVAL previous{};
        previous.Source = 0; // ProfileTime
        ULONG returnLength = 0;
        if (TraceQueryInformation(0, TraceSampledProfileIntervalInfo, &previous,
                                  sizeof(previous), &returnLength) == ERROR_SUCCESS) {
            ctx.previousProfileInterval = previous.Interval;
        }

        TRACE_PROFILE_INTERVAL profileInterval{};
        profileInterval.Source = 0; // ProfileTime
        // Floor at the classic kernel-profiler maximum rate (~8.1 kHz, the
        // xperf/WPR limit). Current kernels accept far smaller intervals
        // verbatim (verified down to 1 µs), but the interval is machine-global
        // and per-CPU — "as fast as possible" (0) must not arm a ~1 MHz
        // interrupt rate on every core.
        const int clampedInterval = qMax(ctx.intervalUs, 123);
        profileInterval.Interval = clampedInterval * 10; // µs → 100-ns units
        const ULONG st = TraceSetInformation(0, TraceSampledProfileIntervalInfo, &profileInterval,
                                             sizeof(profileInterval));
        if (st != ERROR_SUCCESS) {
            return ResultError(Tr::tr("Cannot set the sampling interval to %1 µs (error 0x%2).")
                                   .arg(clampedInterval)
                                   .arg(st, 0, 16));
        }
        qCDebug(etwLog, "Sampling interval set to %d µs.", clampedInterval);
    }

    // --- Start NT Kernel Logger (without flags yet; consumer attaches first) ---
    ULONG status = StartTraceA(&ctx.sessionHandle, loggerName, props);
    if (status != ERROR_SUCCESS) {
        ctx.sessionHandle = 0;
        if (status == ERROR_ACCESS_DENIED) {
            return ResultError(Tr::tr("Access denied when starting the kernel logger. "
                                      "Run Qt Creator as administrator."));
        }
        return ResultError(Tr::tr("Cannot start the NT Kernel Logger (error 0x%1). Another "
                                  "profiler may be using it.").arg(status, 0, 16));
    }
    qCDebug(etwLog, "NT Kernel Logger started (handle=0x%llx).",
            static_cast<unsigned long long>(ctx.sessionHandle));

    // Tear the session down again on every failure below, so a half-started
    // kernel logger is not left behind for the next run to trip over.
    QScopeGuard stopSession([&ctx] {
        controlTrace(ctx.sessionHandle, ctx.sessionProps, EVENT_TRACE_CONTROL_STOP);
        ctx.sessionHandle = 0;
    });

    // --- Open trace consumer (session must be running for real-time mode) ---
    EVENT_TRACE_LOGFILEA traceLog{};
    traceLog.LoggerName = loggerName;
    traceLog.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME
                                | PROCESS_TRACE_MODE_EVENT_RECORD;
    traceLog.EventRecordCallback = onEventRecord;
    traceLog.BufferCallback = onBufferComplete;
    traceLog.Context = &ctx;

    ctx.consumerHandle = OpenTraceA(&traceLog);
    if (ctx.consumerHandle == INVALID_PROCESSTRACE_HANDLE)
        return ResultError(Tr::tr("Cannot attach to the kernel logger (error %1).").arg(GetLastError()));

    QScopeGuard closeConsumer([&ctx] {
        CloseTrace(ctx.consumerHandle);
        ctx.consumerHandle = INVALID_PROCESSTRACE_HANDLE;
    });

    // --- Enable the event classes this backend consumes ---
    // PROFILE delivers the sampled profile events, IMAGE_LOAD tells us when the
    // target's module list changed, and THREAD keeps the kernel's thread
    // bookkeeping (which stack walking builds on) up to date.
    status = controlTrace(ctx.sessionHandle, ctx.sessionProps, EVENT_TRACE_CONTROL_UPDATE,
                          EVENT_TRACE_FLAG_PROFILE | EVENT_TRACE_FLAG_THREAD
                              | EVENT_TRACE_FLAG_IMAGE_LOAD);
    if (status != ERROR_SUCCESS)
        return ResultError(Tr::tr("Cannot enable profiling on the kernel logger (error 0x%1).")
                               .arg(status, 0, 16));

    // --- Enable stack walking for SampledProfile events ---
    {
        CLASSIC_EVENT_ID stackWalkId{};
        stackWalkId.EventGuid = PerfInfoGuid;
        stackWalkId.Type = kSampledProfileOpcode;
        status = TraceSetInformation(ctx.sessionHandle, TraceStackTracingInfo, &stackWalkId,
                                     sizeof(stackWalkId));
        if (status != ERROR_SUCCESS)
            return ResultError(Tr::tr("Cannot enable stack walking (error 0x%1).").arg(status, 0, 16));
    }

    qCDebug(etwLog, "Session ready; stack walking enabled for SampledProfile.");
    closeConsumer.dismiss();
    stopSession.dismiss();
    return ResultOk;
}

// Consume events until the session is stopped from another thread.
void runEtwCapture(EtwCaptureContext &ctx)
{
    ProcessTrace(&ctx.consumerHandle, 1, nullptr, nullptr);
    qCDebug(etwLog, "ProcessTrace returned.");
    CloseTrace(ctx.consumerHandle);
    ctx.consumerHandle = INVALID_PROCESSTRACE_HANDLE;
}

// Stop the ETW session so ProcessTrace returns.
void stopEtwSession(EtwCaptureContext &ctx)
{
    if (ctx.sessionHandle == 0)
        return;
    // Set before stopping: should ControlTrace fail, the buffer callback
    // returning FALSE still ends ProcessTrace at the next buffer.
    ctx.stopRequested.store(true, std::memory_order_relaxed);
    qCDebug(etwLog, "Stopping session (handle=0x%llx, stack fragments=%lld).",
            static_cast<unsigned long long>(ctx.sessionHandle),
            ctx.buffer.get([](const CaptureBuffer &b) {
                return static_cast<long long>(b.fragments.size());
            }));
    const ULONG st = controlTrace(ctx.sessionHandle, ctx.sessionProps, EVENT_TRACE_CONTROL_STOP);
    if (st != ERROR_SUCCESS) {
        qCWarning(etwLog, "ControlTrace(STOP) failed (0x%lx); relying on the buffer "
                          "callback to end the capture.", st);
    }
    ctx.sessionHandle = 0;
}

} // namespace

Result<FilePath> recordSampleTrace(const SamplerOptions &opts, const std::atomic_bool &stop,
                                   std::atomic<int> *progressPercent)
{
    // Resolve target PID.
    DWORD targetPid = 0;
    if (opts.pid > 0) {
        targetPid = static_cast<DWORD>(opts.pid);
    } else {
        const Result<DWORD> found = findProcessByName(opts.processName);
        if (!found)
            return ResultError(found.error());
        targetPid = *found;
    }

    // Open handle for symbolication.
    HANDLE targetProcess =
        OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetPid);
    if (!targetProcess)
        return ResultError(Tr::tr("Cannot open target process (PID %1). "
                                  "Make sure the process is running and you have access.")
                               .arg(targetPid));

    const QScopeGuard closeTarget([&] { CloseHandle(targetProcess); });

    // Elevate privilege for kernel profiling.
    if (!elevateProfilePrivilege())
        return ResultError(Tr::tr("Cannot enable profiling privilege. "
                                  "Run Qt Creator as administrator."));

    // Initialize capture context. Declared after closeTarget so it is destroyed
    // before the process handle it holds is closed.
    EtwCaptureContext ctx(targetProcess, targetPid, opts.intervalUs);

    // The sampled-profile interval is machine-global; put the value found by
    // startEtwSession() back no matter how the recording ends. It fills in the
    // field before changing the interval, so this also covers its error paths.
    const QScopeGuard restoreInterval([&ctx] {
        if (ctx.previousProfileInterval == 0)
            return;
        TRACE_PROFILE_INTERVAL previous{};
        previous.Source = 0; // ProfileTime
        previous.Interval = ctx.previousProfileInterval;
        TraceSetInformation(0, TraceSampledProfileIntervalInfo, &previous, sizeof(previous));
    });

    // Start the session here rather than on the capture thread, so that a stop
    // request arriving immediately cannot overtake the session it has to stop.
    if (Result<> started = startEtwSession(ctx); !started)
        return ResultError(started.error());

    // Consume events in a background thread; wait for stop on this thread.
    std::thread captureThread([&ctx] { runEtwCapture(ctx); });

    // Wait for stop signal from GUI thread.
    while (!stop.load(std::memory_order_relaxed))
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Stop the ETW session so ProcessTrace returns in the capture thread, then
    // wait for the in-flight callbacks to drain.
    stopEtwSession(ctx);
    captureThread.join();

    // The capture thread is gone, so the buffer can be taken over wholesale
    // and everything below runs single-threaded on the complete trace.
    QList<CaptureBuffer::Fragment> fragments;
    ctx.buffer.write([&ctx, &fragments](CaptureBuffer &b) {
        fragments = std::move(b.fragments);
        ctx.data.threadNames = std::move(b.threadNames);
    });

    // Sort by timestamp (ETW events from different CPUs arrive out of order);
    // break ties by thread and user-before-kernel, which lines the two halves
    // of a split kernel-mode sample up next to each other, root portion first.
    std::sort(fragments.begin(), fragments.end(),
              [](const CaptureBuffer::Fragment &a, const CaptureBuffer::Fragment &b) {
                  if (a.rawTs != b.rawTs)
                      return a.rawTs < b.rawTs;
                  if (a.sample.tid != b.sample.tid)
                      return a.sample.tid < b.sample.tid;
                  return !a.kernel && b.kernel;
              });

    // Stitch split kernel/user stacks back into one sample per profile hit
    // (see CaptureBuffer::Fragment): the user stack is the outer (root)
    // portion, the kernel stack goes on top of it. A kernel half without a
    // user half (the thread never returned to user mode) stays as it is.
    ctx.data.samples.reserve(fragments.size());
    for (qsizetype i = 0; i < fragments.size(); ++i) {
        CaptureBuffer::Fragment &fragment = fragments[i];
        if (i + 1 < fragments.size()) {
            const CaptureBuffer::Fragment &next = fragments.at(i + 1);
            if (next.rawTs == fragment.rawTs && next.sample.tid == fragment.sample.tid
                && next.kernel && !fragment.kernel) {
                fragment.sample.frames += next.sample.frames;
                ++i; // the kernel half is merged, not its own sample
            }
        }
        ctx.data.samples.append(std::move(fragment.sample));
    }
    fragments.clear();

    // Post-processing publishes 0..100: symbolication fills the first half of
    // the range, writing the trace the second.
    const auto postProgress = [progressPercent](int percent) {
        if (progressPercent)
            progressPercent->store(percent, std::memory_order_relaxed);
    };

    // Resolve function names and source info for all labels (post-capture).
    // In-capture symbolication is not an option: DbgHelp may load PDBs from
    // disk or a symbol server, which is far too slow for the event callback.
    ctx.symbolicator.resolveLabels(ctx.data,
                                   [&postProgress](int p) { postProgress(p / 2); });

    qCDebug(etwLog, "Capture finished. eventsReceived=%zu, stackWalkEvents=%zu, samples=%lld, "
                    "sampleBytes=%zu, duration=%llu ms.",
            ctx.eventsReceived.load(), ctx.stackWalkEvents.load(),
            static_cast<long long>(ctx.data.samples.size()), ctx.sampleBytes.load(),
            ctx.data.samples.isEmpty() ? 0 : ctx.data.samples.last().tsUs / 1000);

    if (ctx.data.samples.isEmpty())
        return ResultError(Tr::tr("No samples were captured. The target may have exited, "
                                  "or the sampling interval may be too low."));

    // Write trace to disk.
    const QString dirName = u"qtprofiler-sample-%1"_s.arg(QDateTime::currentMSecsSinceEpoch());
    const QString dirPath =
        QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(dirName);
    if (!QDir().mkpath(dirPath))
        return ResultError(Tr::tr("Cannot create temporary trace directory %1.").arg(dirPath));

    // Transfer data ownership and write trace. The process handle is closed by
    // the scope guard when this function returns.
    SampleTraceData data = std::move(ctx.data);

    const FilePath dir = FilePath::fromString(dirPath);
    if (Result<> r = writeSampleTrace(data, dir,
                                      [&postProgress](int p) { postProgress(50 + p / 2); });
        !r) {
        return ResultError(r.error());
    }
    return dir;
}

} // namespace QmlProfiler::Internal
