// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macsampler.h"
#include "sampletrace.h"
#include "symbolicator.h"

#include "profilertr.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVarLengthArray>

#include <vector>

using namespace Profiler;
using namespace Utils;

namespace QmlProfiler::Internal {

#ifndef Q_OS_MACOS

Result<FilePath> recordSampleTrace(const SamplerOptions &, const std::atomic_bool &,
                                   std::atomic<int> *)
{
    return ResultError(Tr::tr("Call-stack sampling is only implemented on macOS."));
}

#else // Q_OS_MACOS

} // namespace QmlProfiler::Internal

#include <libproc.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach/thread_status.h>

#include <time.h>

using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {
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
void capture(task_t task, const SamplerOptions &opts, const std::atomic_bool &stop,
             SampleTraceData &data, LiveLabeler &labeler)
{
    const quint64 startNs = nowNs();
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
        // A failure here means the target task is gone (the process has exited).
        // Stop sampling so the recording finishes: when attached to a process we
        // did not launch ourselves, this is the only signal that the target quit.
        if (task_threads(task, &threads, &threadCount) != KERN_SUCCESS)
            break;

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

    auto taskResult = attachToPid(pid);
    if (!taskResult)
        return ResultError(taskResult.error());
    const task_t task = *taskResult;

    SampleTraceData data;
    data.pid = quint64(pid);

    // Symbolize while the target runs (see LiveLabeler): this picks up images
    // loaded during recording, such as dlopen()'d plugins, and keeps symbols even
    // if the target is force-quit, since nothing is resolved after capture ends.
    Symbolicator symbolicator(task);
    LiveLabeler labeler(task, symbolicator, data);
    capture(task, opts, stop, data, labeler);

    mach_port_deallocate(mach_task_self(), task);

    if (data.samples.isEmpty())
        return ResultError(Tr::tr("No samples were captured. The target may have exited."));

    const QString dirName = u"qmltraceviewer-sample-%1"_s.arg(
        QDateTime::currentMSecsSinceEpoch());
    const QString dirPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                .filePath(dirName);
    if (!QDir().mkpath(dirPath))
        return ResultError(Tr::tr("Cannot create temporary trace directory %1.").arg(dirPath));

    // Symbolication already happened during capture, so the post-stop work is
    // just writing the trace.
    const auto writeProgress = [progressPercent](int percent) {
        if (progressPercent)
            progressPercent->store(percent, std::memory_order_relaxed);
    };
    const FilePath dir = FilePath::fromString(dirPath);
    if (Result<> r = writeSampleTrace(data, dir, writeProgress); !r)
        return ResultError(r.error());
    return dir;
}

#endif // Q_OS_MACOS

} // namespace QmlProfiler::Internal
