/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "uvtargetdevicemodel.h"

#include <utils/algorithm.h>

#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>

using namespace Utils;

namespace BareMetal {
namespace Internal {
namespace Uv {

static QString extractPacksPath(const FilePath &toolsIniFile)
{
    QFile f(toolsIniFile.toString());
    if (!f.open(QIODevice::ReadOnly))
        return {};
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QByteArray line = f.readLine().trimmed();
        const auto startIndex = line.indexOf("RTEPATH=\"");
        const auto stopIndex = line.lastIndexOf('"');
        if (startIndex != 0 || (stopIndex + 1) != line.size())
            continue;
        const QFileInfo path(QString::fromUtf8(line.mid(startIndex + 9,
                                                        stopIndex - startIndex - 9)));
        if (!path.exists())
            return {};
        return path.absoluteFilePath();
    }
    return {};
}

static QString extractPackVersion(const QString &packFilePath)
{
    return QFileInfo(packFilePath).dir().dirName();
}

static QStringList findKeilPackFiles(const QString &path)
{
    QStringList files;
    // Search for the STMicroelectronics devices.
    QDirIterator it(path, {"STM*_DFP"}, QDir::Dirs);
    while (it.hasNext()) {
        const QDir dfpDir(it.next());
        const QFileInfoList entries = dfpDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                           QDir::Name);
        if (entries.isEmpty())
            continue;
        QDirIterator fit(entries.last().absoluteFilePath(), {"*.pdsc"},
                         QDir::Files | QDir::NoSymLinks);
        while (fit.hasNext())
            files.push_back(fit.next());
    }
    return files;
}

static void fillElementProperty(QXmlStreamReader &in, QString &prop)
{
    prop = in.readElementText().trimmed();
}

static void fillCpu(QXmlStreamReader &in, DeviceSelection::Cpu &cpu)
{
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    cpu.core = attrs.value("Dcore").toString();
    cpu.clock = attrs.value("Dclock").toString();
    cpu.fpu = attrs.value("Dfpu").toString();
    cpu.mpu = attrs.value("Dmpu").toString();
}

static void fillMemories(QXmlStreamReader &in, DeviceSelection::Memories &memories)
{
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    DeviceSelection::Memory memory;
    memory.id = attrs.value("id").toString();
    memory.start = attrs.value("start").toString();
    memory.size = attrs.value("size").toString();
    memories.push_back(memory);
}

static void fillAlgorithms(QXmlStreamReader &in, DeviceSelection::Algorithms &algorithms)
{
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    DeviceSelection::Algorithm algorithm;
    algorithm.path = attrs.value("name").toString();
    algorithm.start = attrs.value("start").toString();
    algorithm.size = attrs.value("size").toString();
    algorithms.push_back(algorithm);
}

static void fillVendor(const QString &vendor, QString &vendorName, QString &vendorId)
{
    const auto colonIndex = vendor.lastIndexOf(':');
    vendorName = vendor.mid(0, colonIndex);
    if (colonIndex != -1)
        vendorId = vendor.mid(colonIndex + 1);
}

static void fillVendor(QXmlStreamReader &in, QString &vendorName, QString &vendorId)
{
    QString vendor;
    fillElementProperty(in, vendor);
    fillVendor(vendor, vendorName, vendorId);
}

static void fillSvd(QXmlStreamReader &in, QString &svd)
{
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    svd = attrs.value("svd").toString();
}

// DeviceSelectionItem

class DeviceSelectionItem final : public TreeItem
{
public:
    enum Column { NameColumn, VersionColumn, VendorNameColumn };
    explicit DeviceSelectionItem()
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole) {
            if (column == NameColumn)
                return name;
            else if (column == VersionColumn)
                return version;
            else if (column == VendorNameColumn)
                return vendorName;
        }
        return {};
    }

    Qt::ItemFlags flags(int column) const final
    {
        Q_UNUSED(column)
        return hasChildren() ? Qt::ItemIsEnabled : (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }

    QString desc;
    QString fullPath;
    QString name;
    QString svd;
    QString url;
    QString vendorId;
    QString vendorName;
    QString version;
    DeviceSelection::Algorithms algorithms;
    DeviceSelection::Cpu cpu;
    DeviceSelection::Memories memories;
};

// DeviceSelectionModel

DeviceSelectionModel::DeviceSelectionModel(QObject *parent)
    : TreeModel<DeviceSelectionItem>(parent)
{
    setHeader({tr("Name"), tr("Version"), tr("Vendor")});
}

void DeviceSelectionModel::fillAllPacks(const FilePath &toolsIniFile)
{
    if (m_toolsIniFile == toolsIniFile)
        return;

    clear();

    m_toolsIniFile = toolsIniFile;
    const QString packsPath = extractPacksPath(m_toolsIniFile);
    if (packsPath.isEmpty())
        return;

    QStringList allPackFiles;
    QDirIterator it(packsPath, {"Keil"}, QDir::Dirs | QDir::NoDotAndDotDot);
    while (it.hasNext()) {
        const QString path = it.next();
        if (path.endsWith("/Keil"))
            allPackFiles << findKeilPackFiles(path);
    }

    if (allPackFiles.isEmpty())
        return;
    for (const QString &packFile : qAsConst(allPackFiles))
        parsePackage(packFile);
}

void DeviceSelectionModel::parsePackage(const QString &packageFile)
{
    QFile f(packageFile);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QXmlStreamReader in(&f);
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "package")
            parsePackage(in, packageFile);
        else
            in.skipCurrentElement();
    }
}

void DeviceSelectionModel::parsePackage(QXmlStreamReader &in, const QString &packageFile)
{
    // Create and fill the 'package' item.
    const auto child = new DeviceSelectionItem;
    rootItem()->appendChild(child);
    child->fullPath = packageFile;
    child->version = extractPackVersion(packageFile);
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "name") {
            fillElementProperty(in, child->name);
        } else if (elementName == "description") {
            fillElementProperty(in, child->desc);
        } else if (elementName == "vendor") {
            fillVendor(in, child->vendorName, child->vendorId);
        } else if (elementName == "url") {
            fillElementProperty(in, child->url);
        } else if (elementName == "devices") {
            while (in.readNextStartElement()) {
                const QStringRef elementName = in.name();
                if (elementName == "family")
                    parseFamily(in, child);
                else
                    in.skipCurrentElement();
            }
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseFamily(QXmlStreamReader &in, DeviceSelectionItem *parent)
{
    // Create and fill the 'family' item.
    const auto child = new DeviceSelectionItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->name = attrs.value("Dfamily").toString();
    fillVendor(attrs.value("Dvendor").toString(), child->vendorName, child->vendorId);
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, child->cpu);
        } else if (elementName == "memory") {
            fillMemories(in, child->memories);
        } else if (elementName == "description") {
            fillElementProperty(in, child->desc);
        } else if (elementName == "subFamily") {
            parseSubFamily(in, child);
        } else if (elementName == "device") {
            parseDevice(in, child);
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseSubFamily(QXmlStreamReader &in, DeviceSelectionItem *parent)
{
    // Create and fill the 'sub-family' item.
    const auto child = new DeviceSelectionItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->name = attrs.value("DsubFamily").toString();
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, child->cpu);
        } else if (elementName == "debug") {
            fillSvd(in, child->svd);
        } else if (elementName == "device") {
            parseDevice(in, child);
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseDevice(QXmlStreamReader &in, DeviceSelectionItem *parent)
{
    // Create and fill the 'device' item.
    const auto child = new DeviceSelectionItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->name = attrs.value("Dname").toString();
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, child->cpu);
        } else if (elementName == "memory") {
            fillMemories(in, child->memories);
        } else if (elementName == "algorithm") {
            fillAlgorithms(in, child->algorithms);
        } else if (elementName == "variant") {
            parseDeviceVariant(in, child);
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseDeviceVariant(QXmlStreamReader &in, DeviceSelectionItem *parent)
{
    // Create and fill the 'device-variant' item.
    const auto child = new DeviceSelectionItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->name = attrs.value("Dvariant").toString();
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, child->cpu);
        } else if (elementName == "memory") {
            fillMemories(in, child->memories);
        } else if (elementName == "algorithm") {
            fillAlgorithms(in, child->algorithms);
        } else {
            in.skipCurrentElement();
        }
    }
}

// DeviceSelectionView

DeviceSelectionView::DeviceSelectionView(QWidget *parent)
    : TreeView(parent)
{
    setRootIsDecorated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
}

void DeviceSelectionView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    if (!current.isValid())
        return;
    const auto selectionModel = qobject_cast<DeviceSelectionModel *>(model());
    if (!selectionModel)
        return;
    const DeviceSelectionItem *item = selectionModel->itemForIndex(current);
    if (!item || item->hasChildren())
        return; // We need only in a last 'device' or 'device-variant' item!

    const auto selection = buildSelection(item);
    if (!selection.name.isEmpty())
        emit deviceSelected(selection);
}

DeviceSelection DeviceSelectionView::buildSelection(const DeviceSelectionItem *item) const
{
    DeviceSelection selection;
    // We need to iterate from the lower 'device' or 'device-variant' item to
    // the upper 'package' item to fill a whole information.
    DeviceSelection::Algorithms &algs = selection.algorithms;
    DeviceSelection::Cpu &cpu = selection.cpu;
    DeviceSelection::Memories &mems = selection.memories;
    DeviceSelection::Package &pkg = selection.package;

    do {
        if (selection.name.isEmpty())
            selection.name = item->name;
        else if (selection.subfamily.isEmpty())
            selection.subfamily = item->name;
        else if (selection.family.isEmpty())
            selection.family = item->name;
        else if (pkg.name.isEmpty())
            pkg.name = item->name;

        if (selection.desc.isEmpty())
            selection.desc = item->desc;
        else if (pkg.desc.isEmpty())
            pkg.desc = item->desc;

        if (selection.vendorId.isEmpty())
            selection.vendorId = item->vendorId;
        else if (pkg.vendorId.isEmpty())
            pkg.vendorId = item->vendorId;

        if (selection.vendorName.isEmpty())
            selection.vendorName = item->vendorName;
        else if (pkg.vendorName.isEmpty())
            pkg.vendorName = item->vendorName;

        if (selection.svd.isEmpty())
            selection.svd = item->svd;

        if (cpu.clock.isEmpty())
            cpu.clock = item->cpu.clock;
        if (cpu.core.isEmpty())
            cpu.core = item->cpu.core;
        if (cpu.fpu.isEmpty())
            cpu.fpu = item->cpu.fpu;
        if (cpu.mpu.isEmpty())
            cpu.mpu = item->cpu.mpu;

        if (pkg.file.isEmpty())
            pkg.file = item->fullPath;
        if (pkg.url.isEmpty())
            pkg.url = item->url;
        if (pkg.version.isEmpty())
            pkg.version = item->version;

        // Add only new flash algorithms.
        for (const DeviceSelection::Algorithm &newAlg : item->algorithms) {
            const bool contains = Utils::contains(algs, [&newAlg](const DeviceSelection::Algorithm &existAlg) {
                return newAlg.path == existAlg.path;
            });
            if (!contains)
                algs.push_back(newAlg);
        }

        // Add only new memory regions.
        for (const DeviceSelection::Memory &newMem : item->memories) {
            const bool contains = Utils::contains(mems, [&newMem](const DeviceSelection::Memory &existMem) {
                return newMem.id == existMem.id;
            });
            if (!contains)
                mems.push_back(newMem);
        }

    } while ((item->level() > 1) && (item = static_cast<const DeviceSelectionItem *>(item->parent())));
    return selection;
}

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
