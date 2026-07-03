// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "winsymbolicator.h"

// The Qt Creator PCH undefines these Windows macros; restore them before the
// Windows headers below are pulled in.
#ifdef QTCREATOR_PCH_H
#define CALLBACK __stdcall
#define IN
#define OUT
#endif
#include <qt_windows.h>

#include <Psapi.h>
#include <dbghelp.h>

#include <QFileInfo>

#include <algorithm>
#include <set>

using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

Q_LOGGING_CATEGORY(etwLog, "qtc.profiler.etw", QtWarningMsg)

namespace {

// Module list of `process`. EnumProcessModulesEx is called twice, once to size
// the buffer and once to fill it, and the second call's byte count is the
// authoritative one: when a module unloads in between, the tail of the buffer
// keeps null handles, and GetModuleFileNameExW(nullptr) reports the main
// executable — which would add the target's own binary once per lost module.
std::vector<ModuleEntry> enumerateModuleEntries(HANDLE process)
{
    std::vector<ModuleEntry> entries;

    DWORD cbNeeded = 0;
    if (!EnumProcessModulesEx(process, nullptr, 0, &cbNeeded, LIST_MODULES_ALL))
        return entries;

    std::vector<HMODULE> modules(cbNeeded / sizeof(HMODULE));
    DWORD cbWritten = 0;
    if (!EnumProcessModulesEx(process, modules.data(), cbNeeded, &cbWritten, LIST_MODULES_ALL))
        return entries;

    modules.resize(std::min<size_t>(modules.size(), cbWritten / sizeof(HMODULE)));

    entries.reserve(modules.size());
    for (const HMODULE hmod : modules) {
        wchar_t path[MAX_PATH] = L"";
        if (!GetModuleFileNameExW(process, hmod, path, ARRAYSIZE(path)))
            continue;

        MODULEINFO modInfo{};
        quint64 base = reinterpret_cast<quint64>(hmod);
        quint64 size = 0;
        if (GetModuleInformation(process, hmod, &modInfo, sizeof(modInfo))) {
            base = reinterpret_cast<quint64>(modInfo.lpBaseOfDll);
            size = modInfo.SizeOfImage;
        }

        entries.push_back({std::wstring(path), base, size});
    }
    return entries;
}

// Base name of a module path; "?" when the path has none.
QString moduleName(const std::wstring &path)
{
    const QString name = QFileInfo(QString::fromWCharArray(path.c_str())).fileName();
    return name.isEmpty() ? u"?"_s : name;
}

// Image list for the given modules, sorted ascending by base address so
// addresses can be attributed to a module with a binary search.
std::vector<Image> toImages(const std::vector<ModuleEntry> &entries)
{
    std::vector<Image> images;
    images.reserve(entries.size());
    for (const ModuleEntry &entry : entries)
        images.push_back({entry.base, entry.size, moduleName(entry.path)});

    std::sort(images.begin(), images.end(),
              [](const Image &a, const Image &b) { return a.base < b.base; });
    return images;
}

// Symbol search path: the srv* cache plus the Microsoft symbol server, followed
// by each module's own directory so PDBs sitting next to their DLL are found.
std::wstring symbolSearchPath(const std::vector<ModuleEntry> &entries)
{
    std::wstring searchPath =
        L"srv*%LOCALAPPDATA%\\QtCreator\\symbols*"
        L"https://msdl.microsoft.com/download/symbols";

    // Most modules live in a handful of directories (System32 above all), and
    // the path is a single string handed to DbgHelp, so keep it to one entry
    // per directory.
    std::set<std::wstring> seenDirs;
    for (const ModuleEntry &entry : entries) {
        if (entry.path.size() < 3 || entry.path[1] != L':')
            continue;

        const size_t sep = entry.path.find_last_of(L"\\/");
        if (sep == std::wstring::npos || sep < 2)
            continue;

        const std::wstring dir = entry.path.substr(0, sep);
        if (seenDirs.insert(dir).second) {
            searchPath += L';';
            searchPath += dir;
        }
    }
    return searchPath;
}

} // namespace

void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     QString *module, quint64 *offset)
{
    auto it = std::upper_bound(images.begin(), images.end(), addr,
                               [](quint64 a, const Image &img) { return a < img.base; });
    if (it != images.begin()) {
        --it;
        // An image with unknown extent (size 0) matches everything above its
        // base; otherwise the address must fall inside the image. Without the
        // size check, everything above the lowest image base would match some
        // module — including kernel addresses, which sit above all of them.
        if (it->size == 0 || addr - it->base < it->size) {
            *module = it->name;
            *offset = addr - it->base;
            return;
        }
    }
    *module = QString();
    *offset = addr;
}

void moduleAndOffset(quint64 addr, const std::vector<Image> &images,
                     const std::vector<Image> &kernelImages,
                     QString *module, quint64 *offset)
{
    // First try user-mode images.
    moduleAndOffset(addr, images, module, offset);
    if (!module->isEmpty())
        return;

    // Fall back to kernel-mode images (top of the virtual address space). Their
    // sizes are unknown (EnumDeviceDrivers reports only base addresses), so the
    // nearest lower base wins.
    auto it = std::upper_bound(kernelImages.begin(), kernelImages.end(), addr,
                               [](quint64 a, const Image &img) { return a < img.base; });
    if (it != kernelImages.begin()) {
        --it;
        *module = it->name;
        *offset = addr - it->base;
    }
}

QString demangle(const char *name)
{
    if (!name || !*name)
        return {};
    // UnDecorateSymbolName has no size-query mode (a null buffer fails with
    // ERROR_INVALID_PARAMETER); it writes what fits, NUL-terminated, into the
    // given buffer. MAX_SYM_NAME is DbgHelp's own cap on symbol names, so a
    // fixed buffer of that size never truncates anything DbgHelp returned.
    char buffer[MAX_SYM_NAME + 1];
    const DWORD decoded = UnDecorateSymbolName(name, buffer, ARRAYSIZE(buffer),
                                               UNDNAME_COMPLETE | UNDNAME_NAME_ONLY);
    if (decoded == 0)
        return QString::fromUtf8(name);
    return QString::fromUtf8(buffer);
}

Symbolicator::Symbolicator(HANDLE process)
    : m_process(process)
{
    // SYMOPT_DEFERRED_LOADS loads debug info only for modules that samples
    // actually hit; eagerly loading would pull PDBs (possibly from the symbol
    // server) for every module of the target. FAIL_CRITICAL_ERRORS/NO_PROMPTS
    // keep DbgHelp from raising UI from the worker thread. Not SYMOPT_UNDNAME:
    // demangle() undecorates with UNDNAME_NAME_ONLY, which drops the parameter
    // list that would otherwise dominate the flame graph.
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS
                  | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);

    // DbgHelp keys its per-session state to an arbitrary handle value; it need
    // not be a real handle since modules are loaded explicitly. Using `this`
    // instead of GetCurrentProcess() gives every recording a fresh module list
    // (module bases of consecutive targets may clash) and keeps other DbgHelp
    // users in this process out of our session.
    m_symHandle = reinterpret_cast<HANDLE>(this);
    if (!SymInitialize(m_symHandle, nullptr, FALSE)) {
        qCWarning(etwLog, "SymInitialize failed (error=%lu).", GetLastError());
        m_symHandle = nullptr;
        return;
    }

    cacheKernelDrivers();
    m_valid = true;
}

Symbolicator::~Symbolicator()
{
    if (m_symHandle)
        SymCleanup(m_symHandle);
}

const std::vector<ModuleEntry> &Symbolicator::refreshModules()
{
    std::vector<ModuleEntry> entries = enumerateModuleEntries(m_process);
    if (!entries.empty())
        m_modules = std::move(entries);
    return m_modules;
}

void Symbolicator::loadModuleSymbols()
{
    // One last refresh in case the target is still alive; when it has already
    // exited this falls back to the module list cached during capture.
    const std::vector<ModuleEntry> &entries = refreshModules();
    if (entries.empty()) {
        qCWarning(etwLog, "No modules could be read from the target process; "
                          "samples stay at module+offset.");
        return;
    }

    const std::wstring searchPath = symbolSearchPath(entries);
    if (!SymSetSearchPathW(m_symHandle, searchPath.c_str()))
        qCWarning(etwLog, "SymSetSearchPathW failed (error=%lu).", GetLastError());

    // Register each module at the base it has in the *target*, so SymFromAddrW
    // can be called with raw target addresses. Debug info itself loads lazily,
    // on the first address resolved in a module (SYMOPT_DEFERRED_LOADS) — but
    // that only works when the module's extent is passed along here: the
    // DbgHelp session runs on a pseudo-handle, so DbgHelp cannot size a
    // merely-registered module itself, and addresses in a module of unknown
    // extent fail to resolve with ERROR_MOD_NOT_FOUND.
    for (const ModuleEntry &entry : entries) {
        SymLoadModuleExW(m_symHandle, nullptr,
                         const_cast<wchar_t *>(entry.path.c_str()), nullptr,
                         entry.base, static_cast<DWORD>(entry.size), nullptr, 0);
    }
    qCDebug(etwLog, "Registered %zu modules for symbol loading.", entries.size());
}

void Symbolicator::cacheKernelDrivers()
{
    DWORD cbNeeded = 0;
    if (!EnumDeviceDrivers(nullptr, 0, &cbNeeded))
        return;

    std::vector<LPVOID> bases(cbNeeded / sizeof(LPVOID));
    DWORD cbWritten = 0;
    if (!EnumDeviceDrivers(bases.data(), cbNeeded, &cbWritten))
        return;

    bases.resize(std::min<size_t>(bases.size(), cbWritten / sizeof(LPVOID)));

    for (void *const driverBase : bases) {
        wchar_t name[MAX_PATH] = L"";
        if (!GetDeviceDriverBaseNameW(driverBase, name, ARRAYSIZE(name)))
            continue;

        // EnumDeviceDrivers does not report image sizes; size 0 = extent unknown.
        m_kernelImages.push_back({reinterpret_cast<quint64>(driverBase), 0,
                                  moduleName(std::wstring(name))});
    }

    std::sort(m_kernelImages.begin(), m_kernelImages.end(),
              [](const Image &a, const Image &b) { return a.base < b.base; });
}

void Symbolicator::resolveLabels(SampleTraceData &data, const std::function<void(int)> &progress)
{
    if (!m_valid)
        return;

    loadModuleSymbols();

    // Every label LiveLabeler produced carries the address it came from, and
    // each address maps to exactly one label, so resolving them in order needs
    // no extra bookkeeping.
    alignas(SYMBOL_INFOW) char symBuf[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(WCHAR)];

    const qsizetype count = data.labels.size();
    for (qsizetype i = 0; i < count; ++i) {
        // The first address in each module can stall on a deferred PDB load or
        // symbol-server download, so report progress along the way.
        if (progress && i % 64 == 0)
            progress(int(i * 100 / count));

        SampleTraceData::Label &label = data.labels[i];
        if (label.address == 0)
            continue;

        auto *symbol = reinterpret_cast<SYMBOL_INFOW *>(symBuf);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
        symbol->MaxNameLen = MAX_SYM_NAME;
        ULONG64 displacement = 0;
        if (SymFromAddrW(m_symHandle, label.address, &displacement, symbol)) {
            const QString name =
                demangle(QString::fromWCharArray(symbol->Name).toUtf8().constData());
            if (!name.isEmpty())
                label.name = name;
        }

        IMAGEHLP_LINEW64 lineInfo = {};
        lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);
        ULONG lineDisplacement = 0;
        if (SymGetLineFromAddrW64(m_symHandle, label.address, &lineDisplacement,
                                  &lineInfo)) {
            label.file = QString::fromWCharArray(lineInfo.FileName);
            label.line = int(lineInfo.LineNumber);
        }
    }
    if (progress)
        progress(100);
}

LiveLabeler::LiveLabeler(Symbolicator &symbolicator, SampleTraceData &data)
    : m_symbolicator(symbolicator)
    , m_data(data)
    , m_images(toImages(symbolicator.refreshModules()))
{}

void LiveLabeler::refreshImages()
{
    std::vector<Image> newImages = toImages(m_symbolicator.refreshModules());
    if (newImages.empty())
        return;

    QMutexLocker locker(&m_mutex);
    if (newImages == m_images)
        return;
    m_images = std::move(newImages);
    // Addresses may attribute differently now — in particular those labeled
    // with a bare hex address before the image they belong to was loaded.
    // Existing label ids stay valid through m_labelIds.
    m_labelIdByAddr.clear();
}

int LiveLabeler::labelIdFor(quint64 addr)
{
    QMutexLocker locker(&m_mutex);

    if (auto it = m_labelIdByAddr.constFind(addr); it != m_labelIdByAddr.constEnd())
        return it.value();

    QString module;
    quint64 offset = 0;
    moduleAndOffset(addr, m_images, m_symbolicator.kernelImages(), &module, &offset);

    // Addresses in neither a module of the target process nor a kernel driver
    // (e.g. JIT-generated code) keep a bare hex label, like the macOS backend:
    // dropping the frame would splice its caller and callee together.
    const QString label = module.isEmpty()
                              ? u"0x%1"_s.arg(addr, 0, 16)
                              : u"%1+0x%2"_s.arg(module).arg(offset, 0, 16);

    int id;
    if (auto it = m_labelIds.constFind(label); it != m_labelIds.constEnd()) {
        id = it.value();
    } else {
        id = int(m_data.labels.size());
        m_labelIds.insert(label, id);
        SampleTraceData::Label l;
        l.name = label;
        l.module = module;
        l.offset = offset;
        l.address = addr; // Original address, for post-capture symbol resolution.
        m_data.labels.append(l);
    }
    m_labelIdByAddr.insert(addr, id);
    return id;
}

} // namespace QmlProfiler::Internal
