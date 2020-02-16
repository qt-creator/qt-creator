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

#include "uvproject.h" // for toolsFilePath()
#include "uvtargetdevicemodel.h"

#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>

using namespace Utils;

namespace BareMetal {
namespace Internal {
namespace Uv {

static QString extractPacksPath(const QString &uVisionFilePath)
{
    QFile f(toolsFilePath(uVisionFilePath));
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

static void fillCpu(QXmlStreamReader &in, DeviceSelection::Cpu &cpu)
{
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    cpu.core = attrs.hasAttribute("Dcore") ? attrs.value("Dcore").toString() : cpu.core;
    cpu.clock = attrs.hasAttribute("Dclock") ? attrs.value("Dclock").toString() : cpu.clock;
    cpu.fpu = attrs.hasAttribute("Dfpu") ? attrs.value("Dfpu").toString() : cpu.fpu;
    cpu.mpu = attrs.hasAttribute("Dmpu") ? attrs.value("Dmpu").toString() : cpu.mpu;
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

// DeviceSelectionItem

class DeviceSelectionItem : public TreeItem
{
public:
    enum class Type { Unknown, Package, Family, SubFamily, Device, DeviceVariant };
    enum Column { NameColumn, VersionColumn, VendorColumn };
    explicit DeviceSelectionItem(Type type = Type::Unknown)
        : m_type(type)
    {}

    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole && column == NameColumn)
            return m_name;
        return {};
    }

    Qt::ItemFlags flags(int column) const override
    {
        Q_UNUSED(column)
        return Qt::ItemIsEnabled;
    }

    DeviceSelectionItem *parentPackItem() const
    {
        return static_cast<DeviceSelectionItem *>(parent());
    }

    QString m_name;
    const Type m_type;
};

// PackageItem

class PackageItem final : public DeviceSelectionItem
{
public:
    explicit PackageItem(const QString &file)
        : DeviceSelectionItem(Type::Package), m_file(file), m_version(extractPackVersion(file))
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole) {
            if (column == NameColumn)
                return m_name;
            else if (column == VersionColumn)
                return m_version;
            else if (column == VendorColumn)
                return m_vendor;
        }
        return {};
    }

    QString m_file;
    QString m_version;
    QString m_desc;
    QString m_vendor;
    QString m_url;
};

// FamilyItem

class FamilyItem final : public DeviceSelectionItem
{
public:
    explicit FamilyItem()
        : DeviceSelectionItem(Type::Family)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole) {
            if (column == NameColumn) {
                return m_name;
            } else if (column == VendorColumn) {
                const auto colonIndex = m_vendor.lastIndexOf(':');
                return m_vendor.mid(0, colonIndex);
            }
        }
        return {};
    }

    QString m_desc;
    QString m_vendor;
};

// SubFamilyItem

class SubFamilyItem final : public DeviceSelectionItem
{
public:
    explicit SubFamilyItem()
        : DeviceSelectionItem(Type::SubFamily)
    {}

    QString m_svd;
};

// DeviceItem

class DeviceItem final : public DeviceSelectionItem
{
public:
    explicit DeviceItem()
        : DeviceSelectionItem(Type::Device)
    {}

    Qt::ItemFlags flags(int column) const final
    {
        Q_UNUSED(column)
        return hasChildren() ? Qt::ItemIsEnabled : (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }

    DeviceSelection::Cpu m_cpu;
    DeviceSelection::Memories m_memories;
    DeviceSelection::Algorithms m_algorithms;
};

// DeviceVariantItem

class DeviceVariantItem final : public DeviceSelectionItem
{
public:
    explicit DeviceVariantItem()
        : DeviceSelectionItem(Type::DeviceVariant)
    {}

    Qt::ItemFlags flags(int column) const final
    {
        Q_UNUSED(column)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }
};

// DeviceSelectionModel

DeviceSelectionModel::DeviceSelectionModel(QObject *parent)
    : TreeModel<DeviceSelectionItem>(parent)
{
    setHeader({tr("Name"), tr("Version"), tr("Vendor")});
}

void DeviceSelectionModel::fillAllPacks(const QString &uVisionFilePath)
{
    if (m_uVisionFilePath == uVisionFilePath)
        return;

    clear();

    m_uVisionFilePath = uVisionFilePath;
    const QString packsPath = extractPacksPath(m_uVisionFilePath);
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

void DeviceSelectionModel::parsePackage(QXmlStreamReader &in, const QString &file)
{
    const auto child = new PackageItem(file);
    rootItem()->appendChild(child);
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "name") {
            child->m_name = in.readElementText().trimmed();
        } else if (elementName == "description") {
            child->m_desc = in.readElementText().trimmed();
        } else if (elementName == "vendor") {
            child->m_vendor = in.readElementText().trimmed();
        } else if (elementName == "url") {
            child->m_url = in.readElementText().trimmed();
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
    const auto child = new FamilyItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->m_name = attrs.value("Dfamily").toString();
    child->m_vendor = attrs.value("Dvendor").toString();
    DeviceSelection::Cpu cpu;
    DeviceSelection::Memories memories;
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, cpu);
        } else if (elementName == "memory") {
            fillMemories(in, memories);
        } else if (elementName == "description") {
            child->m_desc = in.readElementText().trimmed();
        } else if (elementName == "subFamily") {
            parseSubFamily(in, child, cpu);
        } else if (elementName == "device") {
            parseDevice(in, child, cpu, memories);
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseSubFamily(QXmlStreamReader &in, DeviceSelectionItem *parent,
                                          DeviceSelection::Cpu &cpu)
{
    const auto child = new SubFamilyItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->m_name = attrs.value("DsubFamily").toString();
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, cpu);
        } else if (elementName == "debug") {
            const QXmlStreamAttributes attrs = in.attributes();
            in.skipCurrentElement();
            child->m_svd = attrs.value("svd").toString();
        } else if (elementName == "device") {
            DeviceSelection::Memories memories;
            parseDevice(in, child, cpu, memories);
        } else {
            in.skipCurrentElement();
        }
    }
}

void DeviceSelectionModel::parseDevice(QXmlStreamReader &in, DeviceSelectionItem *parent,
                                       DeviceSelection::Cpu &cpu,
                                       DeviceSelection::Memories &memories)
{
    const auto child = new DeviceItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    child->m_name = attrs.value("Dname").toString();
    DeviceSelection::Algorithms algorithms;
    while (in.readNextStartElement()) {
        const QStringRef elementName = in.name();
        if (elementName == "processor") {
            fillCpu(in, cpu);
        } else if (elementName == "memory") {
            fillMemories(in, memories);
        } else if (elementName == "algorithm") {
            fillAlgorithms(in, algorithms);
        } else if (elementName == "variant") {
            parseDeviceVariant(in, child);
        } else {
            in.skipCurrentElement();
        }
    }
    child->m_cpu = cpu;
    child->m_memories = memories;
    child->m_algorithms = algorithms;
}

void DeviceSelectionModel::parseDeviceVariant(QXmlStreamReader &in, DeviceSelectionItem *parent)
{
    const auto child = new DeviceVariantItem;
    parent->appendChild(child);
    const QXmlStreamAttributes attrs = in.attributes();
    in.skipCurrentElement();
    child->m_name = attrs.value("Dvariant").toString();
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
    if (isValidItem(item)) {
        const auto selection = buildSelection(item);
        if (!selection.name.isEmpty())
            emit deviceSelected(selection);
    }
}

bool DeviceSelectionView::isValidItem(const DeviceSelectionItem *item) const
{
    if (!item)
        return false;
    if (item->m_type == DeviceSelectionItem::Type::DeviceVariant)
        return true;
    if (item->m_type == DeviceSelectionItem::Type::Device && !item->hasChildren())
        return true;
    return false;
}

DeviceSelection DeviceSelectionView::buildSelection(const DeviceSelectionItem *item) const
{
    DeviceSelection selection;
    // We need to iterate from the lower 'Device|DeviceVariant' items to
    // the upper 'Package' item to fill whole information.
    do {
        switch (item->m_type) {
        case DeviceSelectionItem::Type::DeviceVariant:
            selection.name = item->m_name;
            break;
        case DeviceSelectionItem::Type::Device: {
            const auto deviceItem = static_cast<const DeviceItem *>(item);
            if (!deviceItem->hasChildren())
                selection.name = item->m_name;
            selection.cpu = deviceItem->m_cpu;
            selection.memories = deviceItem->m_memories;
            selection.algorithms = deviceItem->m_algorithms;
        }
            break;
        case DeviceSelectionItem::Type::SubFamily: {
            const auto subFamilyItem = static_cast<const SubFamilyItem *>(item);
            selection.subfamily = subFamilyItem->m_name;
            selection.svd = subFamilyItem->m_svd;
        }
            break;
        case DeviceSelectionItem::Type::Family: {
            const auto familyItem = static_cast<const FamilyItem *>(item);
            selection.family = familyItem->m_name;
            selection.desc = familyItem->m_desc;
            selection.vendor = familyItem->m_vendor;
        }
            break;
        case DeviceSelectionItem::Type::Package: {
            const auto packageItem = static_cast<const PackageItem *>(item);
            selection.package.desc = packageItem->m_desc;
            selection.package.file = packageItem->m_file;
            selection.package.name = packageItem->m_name;
            selection.package.url = packageItem->m_url;
            selection.package.vendor = packageItem->m_vendor;
            selection.package.version = packageItem->m_version;
        }
            break;
        default:
            break;
        }
    } while ((item = item->parentPackItem()));
    return selection;
}

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
