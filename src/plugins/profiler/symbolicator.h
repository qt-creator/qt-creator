// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampletrace.h"

#include <QHash>
#include <QString>

#ifdef Q_OS_MACOS

#include <mach/mach.h>
#include <mach/mach_vm.h>

#include <vector>

namespace QmlProfiler::Internal {

// Reads a single value of type T out of the target task's address space. Returns
// false (and leaves *out untouched) if the read fails or is short. Lives in the
// header so both the sampler (stack walking) and symbolicator (image lists) use
// the same primitive.
template<typename T>
bool readRemote(task_t task, mach_vm_address_t addr, T *out)
{
    mach_vm_size_t got = 0;
    const kern_return_t kr = mach_vm_read_overwrite(
        task, addr, sizeof(T), reinterpret_cast<mach_vm_address_t>(out), &got);
    return kr == KERN_SUCCESS && got == sizeof(T);
}

// A loaded Mach-O image, used to attribute an address to "module+offset".
struct Image
{
    quint64 base = 0;
    QString name;

    friend bool operator==(const Image &, const Image &) = default;
};

// Loaded-image list of `task`, sorted ascending by base address so addresses can
// be attributed to a module with a binary search.
std::vector<Image> readImages(task_t task);

// Number of images dyld currently reports as loaded. Cheap enough to poll every
// tick so the sampler can notice dlopen()/dlclose() and refresh its image list.
uint32_t loadedImageCount(task_t task);

// Module name and offset (addr - image base) for an address. Empty module and
// offset = addr when no loaded image contains it.
void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     QString *module, quint64 *offset);

// "module+0xoffset" (or a bare hex address when no image contains it). Used as
// the fallback when CoreSymbolication has no function symbol for an address.
QString moduleOffsetLabel(quint64 addr, const std::vector<Image> &images);

// Demangles an Itanium C++ ABI symbol name; returns the input unchanged when it
// is not a mangled name. Returns an empty string for null/empty input.
QString demangle(const char *name);

// Resolves addresses to function names via the private CoreSymbolication
// framework (the same engine Instruments/`sample` use), loaded with dlopen so a
// missing framework degrades to module+offset rather than failing to link.
class Symbolicator
{
public:
    explicit Symbolicator(task_t task);
    ~Symbolicator();

    Symbolicator(const Symbolicator &) = delete;
    Symbolicator &operator=(const Symbolicator &) = delete;

    // True once CoreSymbolication is available and a symbolicator was created for
    // the task. When false, name() and sourceInfo() are no-ops and callers should
    // fall back to moduleOffsetLabel().
    bool isValid() const { return !isNull(m_cs); }

    // Demangled function name for an address, or an empty string when no symbol
    // covers it (or the framework is unavailable).
    QString name(quint64 addr) const;

    // Best-effort source path + line for an address. Leaves *path/*line
    // untouched when the framework or debug info is unavailable.
    void sourceInfo(quint64 addr, QString *path, int *line) const;

    // Rebuilds the underlying CoreSymbolication symbolicator so it sees the
    // task's current image set. CSSymbolicatorCreateWithTask snapshots the
    // loaded libraries, and querying "now" does not rescan, so libraries loaded
    // after construction (e.g. dlopen()'d plugins) are invisible until this is
    // called. Cheap to skip when no library has been (un)loaded; the caller
    // gates it on a dyld image-count change.
    void refresh();

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

    task_t m_task = MACH_PORT_NULL;
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

// Resolves target addresses to trace label ids while the target is alive,
// re-reading the loaded-image list when the target's dyld set changes so that
// code in libraries loaded during recording (e.g. dlopen()'d plugins) is
// symbolized too. Resolving live also means symbols survive a force-quit, since
// nothing here depends on the target still existing afterwards.
class LiveLabeler
{
public:
    LiveLabeler(task_t task, Symbolicator &symbolicator, SampleTraceData &data);

    // Re-reads the image list (and refreshes the symbolicator's image snapshot)
    // when dyld reports a different image count, so code in libraries loaded
    // during recording is both attributed to a module and symbolized. Call
    // between sampling ticks, with the target alive.
    void refreshImagesIfChanged();

    // Label id for an address: symbolicates it (caching per address and per
    // resulting label), appending a new SampleTraceData::Label the first time a
    // label is seen.
    int labelIdFor(quint64 addr);

private:
    task_t m_task;
    Symbolicator &m_symbolicator;
    SampleTraceData &m_data;
    std::vector<Image> m_images;
    uint32_t m_imageCount = 0;
    QHash<QString, int> m_labelIds;
    QHash<quint64, int> m_labelIdByAddr;
};

} // namespace QmlProfiler::Internal

#endif // Q_OS_MACOS
