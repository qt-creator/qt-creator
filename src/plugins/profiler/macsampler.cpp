// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macsampler.h"
#include "sampletrace.h"

#include "profilertr.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QStandardPaths>
#include <QVarLengthArray>

#include <algorithm>
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
#include <mach-o/dyld_images.h>

#include <cxxabi.h>
#include <dlfcn.h>
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

// A loaded Mach-O image, used to attribute an address to "module+offset".
struct Image
{
    quint64 base = 0;
    QString name;
};

quint64 nowNs()
{
    return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
}

template<typename T>
bool readRemote(task_t task, mach_vm_address_t addr, T *out)
{
    mach_vm_size_t got = 0;
    const kern_return_t kr = mach_vm_read_overwrite(
        task, addr, sizeof(T), reinterpret_cast<mach_vm_address_t>(out), &got);
    return kr == KERN_SUCCESS && got == sizeof(T);
}

QString readRemoteCString(task_t task, mach_vm_address_t addr)
{
    QByteArray bytes;
    while (bytes.size() < 4096) {
        char chunk[64];
        mach_vm_size_t got = 0;
        if (mach_vm_read_overwrite(task, addr + bytes.size(), sizeof(chunk),
                                   reinterpret_cast<mach_vm_address_t>(chunk), &got)
                != KERN_SUCCESS
            || got == 0) {
            break;
        }
        const int nul = QByteArray(chunk, int(got)).indexOf('\0');
        if (nul >= 0) {
            bytes.append(chunk, nul);
            break;
        }
        bytes.append(chunk, int(got));
    }
    return QString::fromUtf8(bytes);
}

Result<task_t> attachToProcess(const QString &name, pid_t *outPid)
{
    std::vector<pid_t> pids(4096);
    const int bytes = proc_listpids(PROC_ALL_PIDS, 0, pids.data(),
                                    int(pids.size() * sizeof(pid_t)));
    if (bytes <= 0)
        return ResultError(Tr::tr("Cannot enumerate running processes."));

    const int count = bytes / int(sizeof(pid_t));
    pid_t found = 0;
    for (int i = 0; i < count; ++i) {
        const pid_t pid = pids[i];
        if (pid <= 0)
            continue;
        char path[PROC_PIDPATHINFO_MAXSIZE] = {};
        if (proc_pidpath(pid, path, sizeof(path)) <= 0)
            continue;
        if (QFileInfo(QString::fromUtf8(path)).fileName() == name) {
            found = pid;
            break;
        }
    }
    if (found == 0)
        return ResultError(Tr::tr("No running process named \"%1\" was found.").arg(name));

    task_t task = MACH_PORT_NULL;
    const kern_return_t kr = task_for_pid(mach_task_self(), found, &task);
    if (kr != KERN_SUCCESS) {
        return ResultError(
            Tr::tr("task_for_pid(%1) failed: %2. Run the viewer as root or sign it with the "
                   "com.apple.security.cs.debugger entitlement.")
                .arg(found)
                .arg(QString::fromUtf8(mach_error_string(kr))));
    }
    *outPid = found;
    return task;
}

std::vector<Image> readImages(task_t task)
{
    std::vector<Image> images;

    task_dyld_info_data_t dyldInfo;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(task, TASK_DYLD_INFO, reinterpret_cast<task_info_t>(&dyldInfo), &count)
        != KERN_SUCCESS) {
        return images;
    }

    dyld_all_image_infos allInfos{};
    if (!readRemote(task, dyldInfo.all_image_info_addr, &allInfos))
        return images;

    for (uint32_t i = 0; i < allInfos.infoArrayCount; ++i) {
        dyld_image_info info{};
        const mach_vm_address_t entry =
            reinterpret_cast<mach_vm_address_t>(allInfos.infoArray) + i * sizeof(dyld_image_info);
        if (!readRemote(task, entry, &info))
            continue;
        Image img;
        img.base = reinterpret_cast<quint64>(info.imageLoadAddress);
        const QString path =
            readRemoteCString(task, reinterpret_cast<mach_vm_address_t>(info.imageFilePath));
        img.name = path.isEmpty() ? QString("?") : QFileInfo(path).fileName();
        images.push_back(img);
    }

    std::sort(images.begin(), images.end(),
              [](const Image &a, const Image &b) { return a.base < b.base; });
    return images;
}

// Module name and offset (addr - image base) for an address. Empty module and
// offset = addr when no loaded image contains it.
void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     QString *module, quint64 *offset)
{
    auto it = std::upper_bound(images.begin(), images.end(), addr,
                               [](quint64 a, const Image &img) { return a < img.base; });
    if (it != images.begin()) {
        --it;
        *module = it->name;
        *offset = addr - it->base;
    } else {
        *module = QString();
        *offset = addr;
    }
}

// "module+0xoffset" (or a bare hex address when no image contains it). Used as
// the fallback when CoreSymbolication has no function symbol for an address.
QString moduleOffsetLabel(quint64 addr, const std::vector<Image> &images)
{
    QString module;
    quint64 offset = 0;
    moduleAndOffset(addr, images, &module, &offset);
    return module.isEmpty() ? u"0x%1"_s.arg(offset, 0, 16)
                            : u"%1+0x%2"_s.arg(module).arg(offset, 0, 16);
}

QString demangle(const char *name)
{
    if (!name || !*name)
        return {};
    if (name[0] == '_' && name[1] == 'Z') {
        int status = 0;
        if (char *d = abi::__cxa_demangle(name, nullptr, nullptr, &status)) {
            QString result = QString::fromUtf8(d);
            std::free(d);
            return result;
        }
    }
    return QString::fromUtf8(name);
}

// Resolves addresses to function names via the private CoreSymbolication
// framework (the same engine Instruments/`sample` use), loaded with dlopen so a
// missing framework degrades to module+offset rather than failing to link.
class Symbolicator
{
public:
    explicit Symbolicator(task_t task)
    {
        m_lib = dlopen("/System/Library/PrivateFrameworks/CoreSymbolication.framework/"
                       "CoreSymbolication",
                       RTLD_LAZY);
        if (!m_lib)
            return;
        m_createWithTask = reinterpret_cast<CreateWithTaskFn>(
            dlsym(m_lib, "CSSymbolicatorCreateWithTask"));
        m_getSymbol = reinterpret_cast<GetSymbolFn>(
            dlsym(m_lib, "CSSymbolicatorGetSymbolWithAddressAtTime"));
        m_symbolName = reinterpret_cast<SymbolNameFn>(dlsym(m_lib, "CSSymbolGetName"));
        m_release = reinterpret_cast<ReleaseFn>(dlsym(m_lib, "CSRelease"));
        m_getSourceInfo = reinterpret_cast<GetSourceInfoFn>(
            dlsym(m_lib, "CSSymbolicatorGetSourceInfoWithAddressAtTime"));
        m_sourceInfoPath = reinterpret_cast<SourceInfoPathFn>(
            dlsym(m_lib, "CSSourceInfoGetPath"));
        m_sourceInfoLine = reinterpret_cast<SourceInfoLineFn>(
            dlsym(m_lib, "CSSourceInfoGetLineNumber"));
        if (m_createWithTask && m_getSymbol && m_symbolName && m_release)
            m_cs = m_createWithTask(task);
    }

    ~Symbolicator()
    {
        if (m_release && !isNull(m_cs))
            m_release(m_cs);
        if (m_lib)
            dlclose(m_lib);
    }

    Symbolicator(const Symbolicator &) = delete;
    Symbolicator &operator=(const Symbolicator &) = delete;

    bool isValid() const { return !isNull(m_cs); }

    QString name(quint64 addr) const
    {
        if (isNull(m_cs))
            return {};
        const CSTypeRef symbol = m_getSymbol(m_cs, addr, kCSNow);
        if (isNull(symbol))
            return {};
        return demangle(m_symbolName(symbol));
    }

    // Best-effort source path + line for an address. Leaves *path/*line
    // untouched when the framework or debug info is unavailable.
    void sourceInfo(quint64 addr, QString *path, int *line) const
    {
        if (isNull(m_cs) || !m_getSourceInfo || !m_sourceInfoPath || !m_sourceInfoLine)
            return;
        const CSTypeRef info = m_getSourceInfo(m_cs, addr, kCSNow);
        if (isNull(info))
            return;
        if (const char *p = m_sourceInfoPath(info))
            *path = QString::fromUtf8(p);
        *line = m_sourceInfoLine(info);
    }

private:
    // CoreSymbolication passes its handles by value as a pair of pointers.
    struct CSTypeRef
    {
        void *a = nullptr;
        void *b = nullptr;
    };
    static constexpr quint64 kCSNow = 0x8000000000000000ULL;
    static bool isNull(CSTypeRef ref) { return ref.a == nullptr && ref.b == nullptr; }

    using CreateWithTaskFn = CSTypeRef (*)(task_t);
    using GetSymbolFn = CSTypeRef (*)(CSTypeRef, mach_vm_address_t, quint64);
    using SymbolNameFn = const char *(*) (CSTypeRef);
    using ReleaseFn = void (*)(CSTypeRef);
    using GetSourceInfoFn = CSTypeRef (*)(CSTypeRef, mach_vm_address_t, quint64);
    using SourceInfoPathFn = const char *(*) (CSTypeRef);
    using SourceInfoLineFn = int (*)(CSTypeRef);

    void *m_lib = nullptr;
    CSTypeRef m_cs;
    CreateWithTaskFn m_createWithTask = nullptr;
    GetSymbolFn m_getSymbol = nullptr;
    SymbolNameFn m_symbolName = nullptr;
    ReleaseFn m_release = nullptr;
    GetSourceInfoFn m_getSourceInfo = nullptr;
    SourceInfoPathFn m_sourceInfoPath = nullptr;
    SourceInfoLineFn m_sourceInfoLine = nullptr;
};

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

std::vector<Sample> capture(task_t task, const SamplerOptions &opts,
                            const std::atomic_bool &stop,
                            QHash<quint64, QString> &threadNames)
{
    std::vector<Sample> samples;
    const quint64 startNs = nowNs();

    while (!stop.load(std::memory_order_relaxed)) {
        const quint64 elapsedNs = nowNs() - startNs;

        thread_act_array_t threads = nullptr;
        mach_msg_type_number_t threadCount = 0;
        if (task_threads(task, &threads, &threadCount) == KERN_SUCCESS) {
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
            for (mach_msg_type_number_t i = 0; i < threadCount; ++i) {
                thread_identifier_info_data_t idInfo;
                mach_msg_type_number_t idCount = THREAD_IDENTIFIER_INFO_COUNT;
                quint64 tid = i;
                if (thread_info(threads[i], THREAD_IDENTIFIER_INFO,
                                reinterpret_cast<thread_info_t>(&idInfo), &idCount)
                    == KERN_SUCCESS) {
                    tid = idInfo.thread_id;
                }

                if (!threadNames.contains(tid)) {
                    thread_extended_info_data_t extInfo;
                    mach_msg_type_number_t extCount = THREAD_EXTENDED_INFO_COUNT;
                    QString name;
                    if (thread_info(threads[i], THREAD_EXTENDED_INFO,
                                    reinterpret_cast<thread_info_t>(&extInfo), &extCount)
                        == KERN_SUCCESS) {
                        name = QString::fromUtf8(extInfo.pth_name);
                    }
                    threadNames.insert(tid, name);
                }

                Sample sample;
                sample.timestampUs = tsUs;
                sample.tid = tid;
                sample.running = running[i];
                walkThread(task, threads[i], sample);
                if (!sample.frames.empty())
                    samples.push_back(std::move(sample));

                mach_port_deallocate(mach_task_self(), threads[i]);
            }
            task_resume(task);
            mach_vm_deallocate(mach_task_self(), reinterpret_cast<mach_vm_address_t>(threads),
                               threadCount * sizeof(thread_act_t));
        }

        if (opts.intervalUs > 0) {
            // Normalize into seconds + nanoseconds: tv_nsec must stay in [0, 1e9),
            // and the multiplication must not overflow for large user-set intervals.
            const long long ns = static_cast<long long>(opts.intervalUs) * 1000;
            timespec req{time_t(ns / 1'000'000'000), long(ns % 1'000'000'000)};
            nanosleep(&req, nullptr);
        }
    }

    return samples;
}

} // namespace

Result<FilePath> recordSampleTrace(const SamplerOptions &opts, const std::atomic_bool &stop,
                                   std::atomic<int> *progressPercent)
{
    pid_t pid = 0;
    auto taskResult = attachToProcess(opts.processName, &pid);
    if (!taskResult)
        return ResultError(taskResult.error());
    const task_t task = *taskResult;

    const std::vector<Image> images = readImages(task);
    QHash<quint64, QString> threadNames;
    const std::vector<Sample> samples = capture(task, opts, stop, threadNames);

    if (samples.empty()) {
        mach_port_deallocate(mach_task_self(), task);
        return ResultError(Tr::tr("No samples were captured. The target may have exited."));
    }

    // Symbolicate while the target task is still attached, then release it.
    // Symbolicating an address is comparatively expensive, so memoize per
    // address: every distinct return address is resolved only once.
    Symbolicator symbolicator(task);

    SampleTraceData data;
    data.pid = quint64(pid);
    data.threadNames = threadNames;

    QHash<QString, int> labelIds;
    QHash<quint64, int> labelIdByAddr;
    const auto labelIdFor = [&](quint64 addr) -> int {
        if (auto it = labelIdByAddr.constFind(addr); it != labelIdByAddr.constEnd())
            return it.value();
        QString label = symbolicator.name(addr);
        if (label.isEmpty())
            label = moduleOffsetLabel(addr, images);
        int id;
        if (auto it = labelIds.constFind(label); it != labelIds.constEnd()) {
            id = it.value();
        } else {
            id = int(data.labels.size());
            labelIds.insert(label, id);
            SampleTraceData::Label l;
            l.name = label;
            symbolicator.sourceInfo(addr, &l.file, &l.line); // representative location
            moduleAndOffset(addr, images, &l.module, &l.offset);
            data.labels.append(l);
        }
        labelIdByAddr.insert(addr, id);
        return id;
    };

    // Symbolication is the dominant cost; report progress across all samples,
    // reserving the last 30% for writing the trace.
    const int totalSamples = std::max<int>(1, int(samples.size()));
    int processed = 0;
    data.samples.reserve(qsizetype(samples.size()));
    for (const Sample &sample : samples) {
        SampleTraceData::ThreadSample out;
        out.tsUs = sample.timestampUs;
        out.tid = sample.tid;
        out.running = sample.running;
        out.frames.reserve(qsizetype(sample.frames.size()));
        for (auto it = sample.frames.rbegin(); it != sample.frames.rend(); ++it)
            out.frames.append(labelIdFor(*it)); // innermost-first -> root-first
        data.samples.append(std::move(out));
        if (progressPercent)
            progressPercent->store(++processed * 70 / totalSamples, std::memory_order_relaxed);
    }
    mach_port_deallocate(mach_task_self(), task);

    const QString dirName = u"qmltraceviewer-sample-%1"_s.arg(
        QDateTime::currentMSecsSinceEpoch());
    const QString dirPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                                .filePath(dirName);
    if (!QDir().mkpath(dirPath))
        return ResultError(Tr::tr("Cannot create temporary trace directory %1.").arg(dirPath));

    const auto writeProgress = [progressPercent](int percent) {
        if (progressPercent)
            progressPercent->store(70 + percent * 30 / 100, std::memory_order_relaxed);
    };
    const FilePath dir = FilePath::fromString(dirPath);
    if (Result<> r = writeSampleTrace(data, dir, writeProgress); !r)
        return ResultError(r.error());
    return dir;
}

#endif // Q_OS_MACOS

} // namespace QmlProfiler::Internal
