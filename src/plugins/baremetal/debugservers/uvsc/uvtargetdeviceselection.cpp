// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvtargetdeviceselection.h"

#include <baremetal/baremetaltr.h>

#include <QComboBox>
#include <QDataWidgetMapper>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

using namespace Utils;

namespace BareMetal::Internal::Uv {

// Software package data keys.
constexpr char packageDescrKeyC[] = "PackageDescription";
constexpr char packageFileKeyC[] = "PackageFile";
constexpr char packageNameKeyC[] = "PackageName";
constexpr char packageUrlKeyC[] = "PackageUrl";
constexpr char packageVendorNameKeyC[] = "PackageVendorName";
constexpr char packageVendorIdKeyC[] = "PackageVendorId";
constexpr char packageVersionKeyC[] = "PackageVersion";
// Device data keys.
constexpr char deviceNameKeyC[] = "DeviceName";
constexpr char deviceDescrKeyC[] = "DeviceDescription";
constexpr char deviceFamilyKeyC[] = "DeviceFamily";
constexpr char deviceSubFamilyKeyC[] = "DeviceSubFamily";
constexpr char deviceVendorNameKeyC[] = "DeviceVendorName";
constexpr char deviceVendorIdKeyC[] = "DeviceVendorId";
constexpr char deviceSvdKeyC[] = "DeviceSVD";
// Device CPU data keys.
constexpr char deviceClockKeyC[] = "DeviceClock";
constexpr char deviceCoreKeyC[] = "DeviceCore";
constexpr char deviceFpuKeyC[] = "DeviceFPU";
constexpr char deviceMpuKeyC[] = "DeviceMPU";
// Device MEMORY data keys.
constexpr char deviceMemoryKeyC[] = "DeviceMemory";
constexpr char deviceMemoryIdKeyC[] = "DeviceMemoryId";
constexpr char deviceMemoryStartKeyC[] = "DeviceMemoryStart";
constexpr char deviceMemorySizeKeyC[] = "DeviceMemorySize";
// Device ALGORITHM data keys.
constexpr char deviceAlgorithmKeyC[] = "DeviceAlgorithm";
constexpr char deviceAlgorithmPathKeyC[] = "DeviceAlgorithmPath";
constexpr char deviceAlgorithmFlashStartKeyC[] = "DeviceAlgorithmStart";
constexpr char deviceAlgorithmFlashSizeKeyC[] = "DeviceAlgorithmSize";
constexpr char deviceAlgorithmRamStartKeyC[] = "DeviceAlgorithmRamStart";
constexpr char deviceAlgorithmRamSizeKeyC[] = "DeviceAlgorithmRamSize";
constexpr char deviceAlgorithmIndexKeyC[] = "DeviceAlgorithmIndex";

// DeviceSelection

Store DeviceSelection::toMap() const
{
    Store map;
    // Software package.
    map.insert(packageDescrKeyC, package.desc);
    map.insert(packageFileKeyC, package.file);
    map.insert(packageNameKeyC, package.name);
    map.insert(packageUrlKeyC, package.url);
    map.insert(packageVendorNameKeyC, package.vendorName);
    map.insert(packageVendorIdKeyC, package.vendorId);
    map.insert(packageVersionKeyC, package.version);
    // Device.
    map.insert(deviceNameKeyC, name);
    map.insert(deviceDescrKeyC, desc);
    map.insert(deviceFamilyKeyC, family);
    map.insert(deviceSubFamilyKeyC, subfamily);
    map.insert(deviceVendorNameKeyC, vendorName);
    map.insert(deviceVendorIdKeyC, vendorId);
    map.insert(deviceSvdKeyC, svd);
    // Device CPU.
    map.insert(deviceClockKeyC, cpu.clock);
    map.insert(deviceCoreKeyC, cpu.core);
    map.insert(deviceFpuKeyC, cpu.fpu);
    map.insert(deviceMpuKeyC, cpu.mpu);
    // Device MEMORY.
    QVariantList memoryList;
    for (const DeviceSelection::Memory &memory : std::as_const(memories)) {
        Store m;
        m.insert(deviceMemoryIdKeyC, memory.id);
        m.insert(deviceMemoryStartKeyC, memory.start);
        m.insert(deviceMemorySizeKeyC, memory.size);
        memoryList.push_back(variantFromStore(m));
    }
    map.insert(deviceMemoryKeyC, memoryList);
    // Device ALGORITHM.
    QVariantList algorithmList;
    for (const DeviceSelection::Algorithm &algorithm : std::as_const(algorithms)) {
        Store m;
        m.insert(deviceAlgorithmPathKeyC, algorithm.path);
        m.insert(deviceAlgorithmFlashStartKeyC, algorithm.flashStart);
        m.insert(deviceAlgorithmFlashSizeKeyC, algorithm.flashSize);
        m.insert(deviceAlgorithmRamStartKeyC, algorithm.ramStart);
        m.insert(deviceAlgorithmRamSizeKeyC, algorithm.ramSize);
        algorithmList.push_back(variantFromStore(m));
    }
    map.insert(deviceAlgorithmKeyC, algorithmList);
    map.insert(deviceAlgorithmIndexKeyC, algorithmIndex);
    return map;
}

void DeviceSelection::fromMap(const Store &map)
{
    // Software package.
    package.desc = map.value(packageDescrKeyC).toString();
    package.file = map.value(packageFileKeyC).toString();
    package.name = map.value(packageNameKeyC).toString();
    package.url = map.value(packageUrlKeyC).toString();
    package.vendorName = map.value(packageVendorNameKeyC).toString();
    package.vendorId = map.value(packageVendorIdKeyC).toString();
    package.version = map.value(packageVersionKeyC).toString();
    // Device.
    name = map.value(deviceNameKeyC).toString();
    desc = map.value(deviceDescrKeyC).toString();
    family = map.value(deviceFamilyKeyC).toString();
    subfamily = map.value(deviceSubFamilyKeyC).toString();
    vendorName = map.value(deviceVendorNameKeyC).toString();
    vendorId = map.value(deviceVendorIdKeyC).toString();
    svd = map.value(deviceSvdKeyC).toString();
    // Device CPU.
    cpu.clock = map.value(deviceClockKeyC).toString();
    cpu.core = map.value(deviceCoreKeyC).toString();
    cpu.fpu = map.value(deviceFpuKeyC).toString();
    cpu.mpu = map.value(deviceMpuKeyC).toString();
    // Device MEMORY.
    const QVariantList memoryList = map.value(deviceMemoryKeyC).toList();
    for (const auto &entry : memoryList) {
        const auto m = entry.toMap();
        DeviceSelection::Memory memory;
        memory.id = m.value(deviceMemoryIdKeyC).toString();
        memory.start = m.value(deviceMemoryStartKeyC).toString();
        memory.size = m.value(deviceMemorySizeKeyC).toString();
        memories.push_back(memory);
    }
    // Device ALGORITHM.
    algorithmIndex = map.value(deviceAlgorithmIndexKeyC).toInt();
    const QVariantList algorithmList = map.value(deviceAlgorithmKeyC).toList();
    for (const auto &entry : algorithmList) {
        const auto m = entry.toMap();
        DeviceSelection::Algorithm algorithm;
        algorithm.path = m.value(deviceAlgorithmPathKeyC).toString();
        algorithm.flashStart = m.value(deviceAlgorithmFlashStartKeyC).toString();
        algorithm.flashSize = m.value(deviceAlgorithmFlashSizeKeyC).toString();
        algorithm.ramStart = m.value(deviceAlgorithmRamStartKeyC).toString();
        algorithm.ramSize = m.value(deviceAlgorithmRamSizeKeyC).toString();
        algorithms.push_back(algorithm);
    }
}

bool DeviceSelection::Package::operator==(const Package &other) const
{
    return desc == other.desc && file == other.file
            && name == other.name && url == other.url
            && vendorName == other.vendorName && vendorId == other.vendorId
            && version == other.version;
}

bool DeviceSelection::Cpu::operator==(const Cpu &other) const
{
    return core == other.core && clock == other.clock
            && fpu == other.fpu && mpu == other.mpu;
}

bool DeviceSelection::Memory::operator==(const Memory &other) const
{
    return id == other.id && start == other.start && size == other.size;
}

bool DeviceSelection::Algorithm::operator==(const Algorithm &other) const
{
    return path == other.path
            && flashStart == other.flashStart && flashSize == other.flashSize
            && ramStart == other.ramStart && ramSize == other.ramSize;
}

bool DeviceSelection::operator==(const DeviceSelection &other) const
{
    return package == other.package && name == other.name && desc == other.desc
            && family == other.family && subfamily == other.subfamily
            && vendorName == other.vendorName && vendorId == other.vendorId
            && svd == other.svd && cpu == other.cpu
            && memories == other.memories && algorithms == other.algorithms
            && algorithmIndex == other.algorithmIndex;
}

// DeviceSelectionMemoryItem

class DeviceSelectionMemoryItem final : public TreeItem
{
public:
    enum Column { IdColumn, StartColumn, SizeColumn};
    explicit DeviceSelectionMemoryItem(int index, DeviceSelection &selection)
        : m_index(index), m_selection(selection)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const auto &memory = m_selection.memories.at(m_index);
            switch (column) {
            case IdColumn: return memory.id;
            case StartColumn: return memory.start;
            case SizeColumn: return memory.size;
            }
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) final
    {
        if (role == Qt::EditRole) {
            auto &memory = m_selection.memories.at(m_index);
            switch (column) {
            case StartColumn:
                memory.start = data.toString();
                return true;
            case SizeColumn:
                memory.size = data.toString();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const final
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (column == StartColumn || column == SizeColumn)
            flags |= Qt::ItemIsEditable;
        return flags;
    }

private:
    const int m_index;
    DeviceSelection &m_selection;
};

// DeviceSelectionMemoryModel

DeviceSelectionMemoryModel::DeviceSelectionMemoryModel(DeviceSelection &selection, QObject *parent)
    : TreeModel<TreeItem, DeviceSelectionMemoryItem>(parent), m_selection(selection)
{
    setHeader({Tr::tr("ID"), Tr::tr("Start"), Tr::tr("Size")});
    refresh();
}

void DeviceSelectionMemoryModel::refresh()
{
    clear();

    const auto begin = m_selection.memories.begin();
    for (auto it = begin; it < m_selection.memories.end(); ++it) {
        const auto index = std::distance(begin, it);
        const auto item = new DeviceSelectionMemoryItem(index, m_selection);
        rootItem()->appendChild(item);
    }
}

// DeviceSelectionMemoryView

DeviceSelectionMemoryView::DeviceSelectionMemoryView(DeviceSelection &selection, QWidget *parent)
    : TreeView(parent)
{
    setUniformRowHeights(false);
    setRootIsDecorated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);

    const auto model = new DeviceSelectionMemoryModel(selection, this);
    setModel(model);

    connect(model, &DeviceSelectionMemoryModel::dataChanged,
            this, &DeviceSelectionMemoryView::memoryChanged);
}

void DeviceSelectionMemoryView::refresh()
{
    qobject_cast<DeviceSelectionMemoryModel *>(model())->refresh();
}

// DeviceSelectionAlgorithmItem

class DeviceSelectionAlgorithmItem final : public TreeItem
{
public:
    enum Column { PathColumn, FlashStartColumn, FlashSizeColumn, RamStartColumn, RamSizeColumn };
    explicit DeviceSelectionAlgorithmItem(int index, DeviceSelection &selection)
        : m_index(index), m_selection(selection)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            const auto &algorithm = m_selection.algorithms.at(m_index);
            switch (column) {
            case PathColumn: return algorithm.path;
            case FlashStartColumn: return algorithm.flashStart;
            case FlashSizeColumn: return algorithm.flashSize;
            case RamStartColumn: return algorithm.ramStart;
            case RamSizeColumn: return algorithm.ramSize;
            }
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) final
    {
        if (role == Qt::EditRole) {
            auto &algorithm = m_selection.algorithms.at(m_index);
            switch (column) {
            case FlashStartColumn:
                algorithm.flashStart = data.toString();
                return true;
            case FlashSizeColumn:
                algorithm.flashSize = data.toString();
                return true;
            case RamStartColumn:
                algorithm.ramStart = data.toString();
                return true;
            case RamSizeColumn:
                algorithm.ramSize = data.toString();
                return true;
            }
        }
        return false;
    }

    Qt::ItemFlags flags(int column) const final
    {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (column == FlashStartColumn || column == FlashSizeColumn
                || column == RamStartColumn || column == RamSizeColumn) {
            flags |= Qt::ItemIsEditable;
        }
        return flags;
    }

private:
    const int m_index;
    DeviceSelection &m_selection;
};

// DeviceSelectionAlgorithmModel

DeviceSelectionAlgorithmModel::DeviceSelectionAlgorithmModel(DeviceSelection &selection,
                                                             QObject *parent)
    : TreeModel<TreeItem, DeviceSelectionAlgorithmItem>(parent), m_selection(selection)
{
    setHeader({Tr::tr("Name"), Tr::tr("FLASH Start"), Tr::tr("FLASH Size"), Tr::tr("RAM Start"), Tr::tr("RAM Size")});
    refresh();
}

void DeviceSelectionAlgorithmModel::refresh()
{
    clear();

    const auto begin = m_selection.algorithms.begin();
    for (auto it = begin; it < m_selection.algorithms.end(); ++it) {
        const auto index = std::distance(begin, it);
        const auto item = new DeviceSelectionAlgorithmItem(index, m_selection);
        rootItem()->appendChild(item);
    }
}

// DeviceSelectionAlgorithmView

DeviceSelectionAlgorithmView::DeviceSelectionAlgorithmView(DeviceSelection &selection,
                                                           QWidget *parent)
    : QWidget(parent)
{
    const auto model = new DeviceSelectionAlgorithmModel(selection, this);
    const auto layout = new QGridLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_comboBox = new QComboBox;
    m_comboBox->setToolTip(Tr::tr("Algorithm path."));
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBox->setModel(model);
    layout->addWidget(m_comboBox, 0, 0, 1, 0);
    // Add FLASH area settings.
    const auto flashLabel = new QLabel(Tr::tr("FLASH:"));
    layout->addWidget(flashLabel, 1, 0);
    const auto flashStartEdit = new QLineEdit;
    flashStartEdit->setToolTip(Tr::tr("Start address."));
    layout->addWidget(flashStartEdit, 1, 1);
    const auto flashSizeEdit = new QLineEdit;
    flashSizeEdit->setToolTip(Tr::tr("Size."));
    layout->addWidget(flashSizeEdit, 1, 2);
    // Add RAM area settings.
    const auto ramLabel = new QLabel(Tr::tr("RAM:"));
    layout->addWidget(ramLabel, 2, 0);
    const auto ramStartEdit = new QLineEdit;
    ramStartEdit->setToolTip(Tr::tr("Start address."));
    layout->addWidget(ramStartEdit, 2, 1);
    const auto ramSizeEdit = new QLineEdit;
    ramSizeEdit->setToolTip(Tr::tr("Size."));
    layout->addWidget(ramSizeEdit, 2, 2);

    setLayout(layout);

    const auto mapper = new QDataWidgetMapper(this);
    mapper->setModel(model);
    mapper->addMapping(flashStartEdit, DeviceSelectionAlgorithmItem::FlashStartColumn);
    mapper->addMapping(flashSizeEdit, DeviceSelectionAlgorithmItem::FlashSizeColumn);
    mapper->addMapping(ramStartEdit, DeviceSelectionAlgorithmItem::RamStartColumn);
    mapper->addMapping(ramSizeEdit, DeviceSelectionAlgorithmItem::RamSizeColumn);

    connect(m_comboBox, &QComboBox::currentIndexChanged, this, [mapper, this](int index) {
        mapper->setCurrentIndex(index);
        emit algorithmChanged(index);
    });

    connect(model, &DeviceSelectionAlgorithmModel::dataChanged, this, [this] {
        emit algorithmChanged(-1);
    });

    connect(flashStartEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
    connect(flashSizeEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
    connect(ramStartEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
    connect(ramSizeEdit, &QLineEdit::editingFinished, mapper, &QDataWidgetMapper::submit);
}

void DeviceSelectionAlgorithmView::setAlgorithm(int index)
{
    m_comboBox->setCurrentIndex(index);
}

void DeviceSelectionAlgorithmView::refresh()
{
    const QSignalBlocker blocker(this);
    qobject_cast<DeviceSelectionAlgorithmModel *>(m_comboBox->model())->refresh();
}

} // BareMetal::Internal::Uv
