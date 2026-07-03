// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sampletrace.h"

#include <QHash>
#include <QLoggingCategory>
#include <QMutex>
#include <QString>

#include <qt_windows.h>

#include <functional>
#include <string>
#include <vector>

namespace QmlProfiler::Internal {

Q_DECLARE_LOGGING_CATEGORY(etwLog)

// A loaded PE image, used to attribute an address to "module+offset".
struct Image
{
    quint64 base = 0;
    quint64 size = 0; // 0 = extent unknown; any address above `base` matches
    QString name;

    friend bool operator==(const Image &, const Image &) = default;
};

// A module loaded in the target process.
struct ModuleEntry
{
    std::wstring path;
    quint64 base = 0;
    quint64 size = 0; // 0 when GetModuleInformation failed
};

// Module name and offset (addr - image base) for an address. Empty module and
// offset = addr when no loaded image contains it (an image contains the
// addresses in [base, base + size), or everything above base if its size is
// unknown).
void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     QString *module, quint64 *offset);

// Overload that also checks kernel-mode images as a fallback.
void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     const std::vector<Image> &kernelImages,
                     QString *module, quint64 *offset);

// Demangles a Microsoft C++ decorated name; returns the input unchanged when
// it is not a mangled name. Returns an empty string for null/empty input.
QString demangle(const char *name);

// Resolves addresses to function names via DbgHelp (the same engine Visual
// Studio and WinDbg use). Each instance runs its own DbgHelp session, keyed to
// a private handle, so consecutive recordings do not inherit each other's
// module lists (the targets' bases may clash) and other DbgHelp users in this
// process (e.g. the crash handler) are left alone. The module list is obtained
// from the target via EnumProcessModulesEx.
class Symbolicator
{
public:
    explicit Symbolicator(HANDLE process);
    ~Symbolicator();

    Symbolicator(const Symbolicator &) = delete;
    Symbolicator &operator=(const Symbolicator &) = delete;

    // Post-capture symbol resolution: points DbgHelp at the target's modules,
    // then resolves every label that carries an address via
    // SymFromAddrW/SymGetLineFromAddrW64, filling in function name and source
    // info. DbgHelp is single-threaded, so this must not run while the ETW
    // callback thread is active; call it once the capture thread has joined.
    // The target may already have exited by then: symbols are loaded from the
    // module list cached by the last successful refreshModules() call.
    // `progress`, if set, is called with 0..100 — debug info loads lazily, so
    // resolving can stall on PDB loads (or symbol-server downloads).
    void resolveLabels(SampleTraceData &data,
                       const std::function<void(int)> &progress = {});

    // Re-reads the target's module list and returns it. When enumeration fails
    // (typically because the target has exited), the previous snapshot is kept
    // and returned instead — the modules it had loaded are still what the
    // samples refer to. Not thread-safe: during capture only the ProcessTrace
    // thread may call this (via LiveLabeler::refreshImages()).
    const std::vector<ModuleEntry> &refreshModules();

    // Returns the kernel-mode image list populated by cacheKernelDrivers().
    const std::vector<Image> &kernelImages() const { return m_kernelImages; }

private:
    void loadModuleSymbols();
    void cacheKernelDrivers();

    HANDLE m_process = nullptr;
    HANDLE m_symHandle = nullptr; // DbgHelp session key, not a real handle
    std::vector<ModuleEntry> m_modules; // last successful enumeration
    std::vector<Image> m_kernelImages;
    bool m_valid = false;
};

// Resolves target addresses to trace label ids while the target is alive,
// re-reading the module list when the target loads an image so that code in
// libraries loaded during recording (e.g. LoadLibrary'd plugins) is attributed
// too. Labels are "module+0xoffset" here (a bare hex address when no image
// contains the address, like the macOS backend); resolveLabels() turns them
// into function names once the capture has stopped.
class LiveLabeler
{
public:
    LiveLabeler(Symbolicator &symbolicator, SampleTraceData &data);

    // Re-reads the target's module list (through the symbolicator, which caches
    // it for post-capture symbol loading). Call between sampling ticks, with
    // the target alive, when an ImageLoad event reports a new image.
    void refreshImages();

    // Label id for an address: attributes it to a module (caching per address
    // and per resulting label), appending a new SampleTraceData::Label the
    // first time a label is seen. Addresses outside every known user-mode and
    // kernel-mode image (e.g. JIT-generated code) get a bare hex-address
    // label. Thread-safe: uses internal mutex.
    int labelIdFor(quint64 addr);

private:
    Symbolicator &m_symbolicator;
    SampleTraceData &m_data;
    std::vector<Image> m_images;
    QHash<QString, int> m_labelIds;
    QHash<quint64, int> m_labelIdByAddr;
    QMutex m_mutex; // Protects all internal state from concurrent ETW callbacks.
};

} // namespace QmlProfiler::Internal
