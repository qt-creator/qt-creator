// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macsampler.h"

#include "profilertr.h"

#ifdef Q_OS_MACOS

#include "sampletrace.h"
#include "symbolicator.h"

#include <QFileInfo>
#include <QScopeGuard>
#include <QVarLengthArray>

#include <vector>

using namespace Utils;

#include <Security/Security.h>

#include <errno.h>
#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_status.h>

#include <signal.h>
#include <time.h>

using namespace Qt::StringLiterals;

namespace Profiler::Internal {
namespace {

constexpr int kMaxStackDepth = 256;
// Top 16 bits of a 64-bit return address hold the pointer-authentication code
// on arm64e; mask them off so the address maps back to a loaded image.
constexpr quint64 kVaMask = 0x0000FFFFFFFFFFFFULL;

// One stack sample of one thread. `frames` is innermost-first (frame[0] is the
// currently executing function).
struct Sample
{
    quint64 timestampUs = 0;
    quint64 tid = 0;
    // Scheduler run state at capture: TH_STATE_RUNNING means runnable/running,
    // read under task_suspend, so it is a best-effort "on-CPU" indicator.
    bool running = false;
    std::vector<quint64> frames;
};

quint64 nowNs()
{
    return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
}

Result<pid_t> findProcessByName(const QString &name)
{
    std::vector<pid_t> pids(4096);
    const int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                                    int(pids.size() * sizeof(pid_t)));
    if (bytes <= 0)
        return ResultError(Tr::tr("Cannot enumerate running processes."));

    const int count = bytes / int(sizeof(pid_t));
    for (int i = 0; i < count; ++i) {
        const pid_t pid = pids[i];
        if (pid <= 0)
            continue;
        char path[PROC_PIDPATHINFO_MAXSIZE] = {};
        if (proc_pidpath(pid, path, sizeof(path)) <= 0)
            continue;
        if (QFileInfo(QString::fromUtf8(path)).fileName() == name)
            return pid;
    }
    return ResultError(Tr::tr("No running process named \"%1\" was found.").arg(name));
}

bool isProcessAlive(pid_t pid)
{
    return kill(pid, 0) == 0 || errno == EPERM;
}

Result<task_t> attachToPid(pid_t pid)
{
    task_t task = MACH_PORT_NULL;
    const kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
    if (kr != KERN_SUCCESS) {
        return ResultError(
            Tr::tr("task_for_pid(%1) failed: %2. Run the viewer as root or sign it with the "
                   "com.apple.security.cs.debugger entitlement.")
                .arg(pid)
                .arg(QString::fromUtf8(mach_error_string(kr))));
    }
    return task;
}

void walkThread(task_t task, thread_act_t thread, Sample &sample)
{
#if defined(__arm64__) || defined(__aarch64__)
    arm_thread_state64_t state;
    mach_msg_type_number_t sc = ARM_THREAD_STATE64_COUNT;
    if (thread_get_state(thread, ARM_THREAD_STATE64,
                         reinterpret_cast<thread_state_t>(&state), &sc)
        != KERN_SUCCESS) {
        return;
    }
    quint64 pc = arm_thread_state64_get_pc(state);
    quint64 fp = arm_thread_state64_get_fp(state);
#elif defined(__x86_64__)
    x86_thread_state64_t state;
    mach_msg_type_number_t sc = x86_THREAD_STATE64_COUNT;
    if (thread_get_state(thread, x86_THREAD_STATE64,
                         reinterpret_cast<thread_state_t>(&state), &sc)
        != KERN_SUCCESS) {
        return;
    }
    quint64 pc = state.__rip;
    quint64 fp = state.__rbp;
#else
    return;
#endif

    sample.frames.push_back(pc & kVaMask);

    for (int depth = 0; depth < kMaxStackDepth && fp; ++depth) {
        if (fp & 0xf)
            break;
        quint64 frame[2]; // [0] = saved frame pointer, [1] = return address
        if (!readRemote(task, fp, &frame))
            break;
        const quint64 ret = frame[1] & kVaMask;
        if (!ret)
            break;
        sample.frames.push_back(ret);
        if (frame[0] <= fp) // the stack grows downward; the saved fp must be higher
            break;
        fp = frame[0];
    }
}

// Samples the target until `stop` is set or it exits, resolving each stack into
// trace labels via `labeler` as it is taken (so symbolication happens while the
// target is alive) and appending the result to `data`.
//
// Returns true when it stopped because the Mach task went away, which is either
// the process exiting or it exec()ing a new image.
//
// Sample timestamps are relative to `startNs`, which is the whole recording's
// zero rather than this call's: a capture that starts over after an exec has to
// stay on the timeline the caller anchored, or a combined recording could not
// line the two traces up (see RecordingSession::markStarted).
bool capture(task_t task, const SamplerOptions &opts, const std::atomic_bool &stop,
             SampleTraceData &data, LiveLabeler &labeler, quint64 startNs)
{
    std::vector<Sample> tick; // raw stacks gathered during one suspend window

    // Samples accumulate in RAM until the trace is written at the end (see
    // writeSampleTrace). At the default 200 us cadence a busy multi-threaded
    // target produces them quickly, so stop once the collected samples reach this
    // budget rather than letting a long session grow without bound. Streaming
    // straight to disk during capture could lift this cap later.
    constexpr size_t kMaxSampleBytes = 512ull * 1024 * 1024;
    size_t sampleBytes = 0;

    while (!stop.load(std::memory_order_relaxed)) {
        const quint64 elapsedNs = nowNs() - startNs;

        thread_act_array_t threads = nullptr;
        mach_msg_type_number_t threadCount = 0;
        // A failure here means the target task is gone. Stop sampling so the
        // recording finishes: when attached to a process we did not launch
        // ourselves, this is the only signal that the target quit.
        if (task_threads(task, &threads, &threadCount) != KERN_SUCCESS)
            return true;

        // Sample each thread's run state from the still-running task: once
        // the task is suspended every thread reports TH_STATE_WAITING, so
        // the on-CPU flag has to be read before suspending. The stack walk
        // below still needs the task suspended.
        QVarLengthArray<bool, 32> running(threadCount);
        for (mach_msg_type_number_t i = 0; i < threadCount; ++i) {
            thread_basic_info_data_t basicInfo;
            mach_msg_type_number_t basicCount = THREAD_BASIC_INFO_COUNT;
            running[i] = thread_info(threads[i], THREAD_BASIC_INFO,
                                     reinterpret_cast<thread_info_t>(&basicInfo), &basicCount)
                             == KERN_SUCCESS
                         && basicInfo.run_state == TH_STATE_RUNNING;
        }

        task_suspend(task);
        const quint64 tsUs = elapsedNs / 1000;
        tick.clear();
        for (mach_msg_type_number_t i = 0; i < threadCount; ++i) {
            thread_identifier_info_data_t idInfo;
            mach_msg_type_number_t idCount = THREAD_IDENTIFIER_INFO_COUNT;
            quint64 tid = i;
            if (thread_info(threads[i], THREAD_IDENTIFIER_INFO,
                            reinterpret_cast<thread_info_t>(&idInfo), &idCount)
                == KERN_SUCCESS) {
                tid = idInfo.thread_id;
            }

            if (!data.threadNames.contains(tid)) {
                thread_extended_info_data_t extInfo;
                mach_msg_type_number_t extCount = THREAD_EXTENDED_INFO_COUNT;
                QString name;
                if (thread_info(threads[i], THREAD_EXTENDED_INFO,
                                reinterpret_cast<thread_info_t>(&extInfo), &extCount)
                    == KERN_SUCCESS) {
                    name = QString::fromUtf8(extInfo.pth_name);
                }
                data.threadNames.insert(tid, name);
            }

            Sample sample;
            sample.timestampUs = tsUs;
            sample.tid = tid;
            sample.running = running[i];
            walkThread(task, threads[i], sample);
            if (!sample.frames.empty())
                tick.push_back(std::move(sample));

            mach_port_deallocate(mach_task_self(), threads[i]);
        }
        // Resolve while the target is still suspended, so CoreSymbolication reads
        // a stable target. Resolving in the brief window after resume (with the
        // next suspend imminent) raced the running target and made symbol lookups
        // fail intermittently. New addresses are resolved once and cached, so
        // steady-state suspend windows stay short; images loaded during recording
        // (e.g. dlopen()'d plugins) are picked up by refreshImagesIfChanged().
        labeler.refreshImagesIfChanged();
        for (Sample &sample : tick) {
            SampleTraceData::ThreadSample out;
            out.tsUs = sample.timestampUs;
            out.tid = sample.tid;
            out.running = sample.running;
            out.frames.reserve(qsizetype(sample.frames.size()));
            for (auto it = sample.frames.rbegin(); it != sample.frames.rend(); ++it)
                out.frames.append(labeler.labelIdFor(*it)); // innermost-first -> root-first
            sampleBytes += sizeof(SampleTraceData::ThreadSample)
                           + size_t(out.frames.size()) * sizeof(int);
            data.samples.append(std::move(out));
        }

        task_resume(task);
        mach_vm_deallocate(mach_task_self(), reinterpret_cast<mach_vm_address_t>(threads),
                           threadCount * sizeof(thread_act_t));

        // Stop cleanly when the memory budget is reached, keeping what was
        // captured (the same outcome as the target exiting mid-recording).
        if (sampleBytes >= kMaxSampleBytes) {
            qWarning("QmlProfiler: reached the %llu MiB sample budget; stopping capture.",
                     static_cast<unsigned long long>(kMaxSampleBytes >> 20));
            break;
        }

        if (opts.intervalUs > 0) {
            // Normalize into seconds + nanoseconds: tv_nsec must stay in [0, 1e9),
            // and the multiplication must not overflow for large user-set intervals.
            const long long ns = static_cast<long long>(opts.intervalUs) * 1000;
            timespec req{time_t(ns / 1'000'000'000), long(ns % 1'000'000'000)};
            nanosleep(&req, nullptr);
        }
    }
    return false;
}

// The task an exec() leaves behind is not usable, and the one that replaces it
// takes a moment to appear.
Result<task_t> waitForNewTask(pid_t pid)
{
    constexpr int attempts = 50; // ~500 ms at the 10 ms cadence below.
    Result<task_t> task = ResultError(Tr::tr("The process exited before it could be "
                                             "attached to again."));
    for (int i = 0; i < attempts && isProcessAlive(pid); ++i) {
        task = attachToPid(pid);
        if (task)
            return task;
        timespec req{0, 10 * 1000 * 1000};
        nanosleep(&req, nullptr);
    }
    return task;
}

} // namespace

Result<FilePath> recordSampleTrace(const SamplerOptions &opts, const std::atomic_bool &stop,
                                   std::atomic<int> *progressPercent)
{
    pid_t pid = 0;
    if (opts.pid > 0) {
        pid = pid_t(opts.pid);
    } else {
        const Result<pid_t> found = findProcessByName(opts.processName);
        if (!found)
            return ResultError(found.error());
        pid = *found;
    }

    Result<task_t> taskResult = attachToPid(pid);
    if (!taskResult)
        return ResultError(taskResult.error());

    SampleTraceData data;
    data.pid = quint64(pid);
    const quint64 startNs = nowNs();

    // What Qt Creator runs is started through a helper that exec()s the real
    // target (see src/tools/disclaim), and an exec replaces the Mach task the
    // sampler holds. Attach to the new one and carry on against the same trace:
    // by this point every frame is a resolved string, so the image it came from
    // being gone does not make it wrong, and starting over would throw away a
    // whole recording when a target exec()s late rather than at startup.
    for (;;) {
        const task_t task = *taskResult;

        // Symbolize while the target runs (see LiveLabeler): this picks up images
        // loaded during recording, such as dlopen()'d plugins, and keeps symbols
        // even if the target is force-quit, since nothing is resolved after
        // capture ends.
        Symbolicator symbolicator(task);
        LiveLabeler labeler(task, symbolicator, data);
        const bool taskGone = capture(task, opts, stop, data, labeler, startNs);

        mach_port_deallocate(mach_task_self(), task);

        if (!taskGone || stop.load(std::memory_order_relaxed) || !isProcessAlive(pid))
            break;
        taskResult = waitForNewTask(pid);
        if (!taskResult)
            break; // The process went away while we were looking for its new task.
    }

    if (data.samples.isEmpty())
        return ResultError(Tr::tr("No samples were captured. The target may have exited."));

    const FilePath dir = uniqueTracePath("qtprofiler-sample"_L1);
    if (!dir.createDir()) {
        return ResultError(
            Tr::tr("Cannot create temporary trace directory %1.").arg(dir.toUserOutput()));
    }

    // Symbolication already happened during capture, so the post-stop work is
    // just writing the trace.
    const auto writeProgress = [progressPercent](int percent) {
        if (progressPercent)
            progressPercent->store(percent, std::memory_order_relaxed);
    };
    if (Result<> r = writeSampleTrace(data, dir, writeProgress); !r)
        return ResultError(r.error());
    return dir;
}

bool canSampleOtherProcesses()
{
    // root may sample anything; everyone else needs the entitlement.
    if (geteuid() == 0)
        return true;

    SecCodeRef self = nullptr;
    if (SecCodeCopySelf(kSecCSDefaultFlags, &self) != errSecSuccess)
        return false;
    const QScopeGuard releaseSelf([self] { CFRelease(self); });

    CFDictionaryRef info = nullptr;
    if (SecCodeCopySigningInformation(reinterpret_cast<SecStaticCodeRef>(self),
                                      kSecCSRequirementInformation, &info) != errSecSuccess) {
        return false;
    }
    const QScopeGuard releaseInfo([info] { CFRelease(info); });

    const auto entitlements = static_cast<CFDictionaryRef>(
        CFDictionaryGetValue(info, kSecCodeInfoEntitlementsDict));
    if (!entitlements)
        return false;
    const auto granted = static_cast<CFBooleanRef>(
        CFDictionaryGetValue(entitlements, CFSTR("com.apple.security.cs.debugger")));
    return granted && CFBooleanGetValue(granted);
}

} // namespace Profiler::Internal

#endif // Q_OS_MACOS

