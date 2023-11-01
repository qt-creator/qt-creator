// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/basetreeview.h>
#include <utils/treemodel.h>
#include <utils/store.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace BareMetal::Internal::Uv {

// DeviceSelection

class DeviceSelection final
{
public:
    struct Package {
        QString desc;
        QString file;
        QString name;
        QString url;
        QString vendorId;
        QString vendorName;
        QString version;

        bool operator==(const Package &other) const;
    };

    struct Cpu {
        QString clock;
        QString core;
        QString fpu;
        QString mpu;

        bool operator==(const Cpu &other) const;
    };

    struct Memory {
        QString id;
        QString size;
        QString start;

        bool operator==(const Memory &other) const;
    };
    using Memories = std::vector<Memory>;

    struct Algorithm {
        QString path;
        QString flashSize;
        QString flashStart;
        QString ramSize;
        QString ramStart;

        bool operator==(const Algorithm &other) const;
    };
    using Algorithms = std::vector<Algorithm>;

    Package package;
    QString name;
    QString desc;
    QString family;
    QString subfamily;
    QString vendorId;
    QString vendorName;
    QString svd;
    Cpu cpu;
    Memories memories;
    Algorithms algorithms;
    int algorithmIndex = 0;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);
    bool operator==(const DeviceSelection &other) const;
};

// DeviceSelectionMemoryModel

class DeviceSelectionMemoryItem;
class DeviceSelectionMemoryModel final
        : public Utils::TreeModel<Utils::TreeItem, DeviceSelectionMemoryItem>
{
    Q_OBJECT

public:
    explicit DeviceSelectionMemoryModel(DeviceSelection &selection, QObject *parent = nullptr);
    void refresh();

private:
    DeviceSelection &m_selection;
};

// DeviceSelectionMemoryView

class DeviceSelectionMemoryView final : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit DeviceSelectionMemoryView(DeviceSelection &selection, QWidget *parent = nullptr);
    void refresh();

signals:
    void memoryChanged();
};

// DeviceSelectionAlgorithmModel

class DeviceSelectionAlgorithmItem;
class DeviceSelectionAlgorithmModel final
        : public Utils::TreeModel<Utils::TreeItem, DeviceSelectionAlgorithmItem>
{
    Q_OBJECT

public:
    explicit DeviceSelectionAlgorithmModel(DeviceSelection &selection, QObject *parent = nullptr);
    void refresh();

private:
    DeviceSelection &m_selection;
};

// DeviceSelectionAlgorithmView

class DeviceSelectionAlgorithmView final : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceSelectionAlgorithmView(DeviceSelection &selection, QWidget *parent = nullptr);
    void setAlgorithm(int index);
    void refresh();

signals:
    void algorithmChanged(int index = -1);

private:
    QComboBox *m_comboBox = nullptr;
};

} // BareMetal::Internal::Uv

Q_DECLARE_METATYPE(BareMetal::Internal::Uv::DeviceSelection)
