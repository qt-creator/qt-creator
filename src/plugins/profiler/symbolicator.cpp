// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "symbolicator.h"

#ifdef Q_OS_MACOS

#include <QByteArray>
#include <QFileInfo>

#include <mach/mach_vm.h>
#include <mach/task.h>
#include <mach-o/dyld_images.h>

#include <cxxabi.h>
#include <dlfcn.h>

#include <algorithm>
#include <cstdlib>

using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {
namespace {

// Reads a NUL-terminated C string out of the target task, up to a sane cap.
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

} // namespace

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

uint32_t loadedImageCount(task_t task)
{
    task_dyld_info_data_t dyldInfo;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    if (task_info(task, TASK_DYLD_INFO, reinterpret_cast<task_info_t>(&dyldInfo), &count)
        != KERN_SUCCESS) {
        return 0;
    }
    dyld_all_image_infos allInfos{};
    if (!readRemote(task, dyldInfo.all_image_info_addr, &allInfos))
        return 0;
    return allInfos.infoArrayCount;
}

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

Symbolicator::Symbolicator(task_t task)
    : m_task(task)
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

Symbolicator::~Symbolicator()
{
    if (m_release && !isNull(m_cs))
        m_release(m_cs);
    if (m_lib)
        dlclose(m_lib);
}

void Symbolicator::refresh()
{
    if (!m_createWithTask)
        return;
    if (m_release && !isNull(m_cs))
        m_release(m_cs);
    m_cs = m_createWithTask(m_task);
}

QString Symbolicator::name(quint64 addr) const
{
    if (isNull(m_cs))
        return {};
    const CSTypeRef symbol = m_getSymbol(m_cs, addr, kCSNow);
    if (isNull(symbol))
        return {};
    return demangle(m_symbolName(symbol));
}

void Symbolicator::sourceInfo(quint64 addr, QString *path, int *line) const
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

LiveLabeler::LiveLabeler(task_t task, Symbolicator &symbolicator, SampleTraceData &data)
    : m_task(task)
    , m_symbolicator(symbolicator)
    , m_data(data)
    , m_images(readImages(task))
    , m_imageCount(loadedImageCount(task))
{}

void LiveLabeler::refreshImagesIfChanged()
{
    const uint32_t now = loadedImageCount(m_task);
    if (now != m_imageCount) {
        m_images = readImages(m_task);
        m_imageCount = now;
        // Keep the symbolicator's image snapshot in sync so functions in
        // newly loaded libraries resolve to names, not just module+offset.
        m_symbolicator.refresh();
    }
}

int LiveLabeler::labelIdFor(quint64 addr)
{
    if (auto it = m_labelIdByAddr.constFind(addr); it != m_labelIdByAddr.constEnd())
        return it.value();
    QString label = m_symbolicator.name(addr);
    if (label.isEmpty())
        label = moduleOffsetLabel(addr, m_images);
    int id;
    if (auto it = m_labelIds.constFind(label); it != m_labelIds.constEnd()) {
        id = it.value();
    } else {
        id = int(m_data.labels.size());
        m_labelIds.insert(label, id);
        SampleTraceData::Label l;
        l.name = label;
        m_symbolicator.sourceInfo(addr, &l.file, &l.line); // representative location
        moduleAndOffset(addr, m_images, &l.module, &l.offset);
        m_data.labels.append(l);
    }
    m_labelIdByAddr.insert(addr, id);
    return id;
}

} // namespace QmlProfiler::Internal

#endif // Q_OS_MACOS
