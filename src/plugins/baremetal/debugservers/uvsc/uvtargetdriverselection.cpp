// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uvtargetdriverselection.h"

#include <baremetal/baremetaltr.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

using namespace Utils;

namespace BareMetal::Internal::Uv {

// Driver data keys.
constexpr char driverIndexKeyC[] = "DriverIndex";
constexpr char driverCpuDllIndexKeyC[] = "DriverCpuDllIndex";
constexpr char driverDllKeyC[] = "DriverDll";
constexpr char driverCpuDllsKeyC[] = "DriverCpuDlls";
constexpr char driverNameKeyC[] = "DriverName";

// DriverSelection

Store DriverSelection::toMap() const
{
    Store map;
    map.insert(driverIndexKeyC, index);
    map.insert(driverCpuDllIndexKeyC, cpuDllIndex);
    map.insert(driverDllKeyC, dll);
    map.insert(driverCpuDllsKeyC, cpuDlls);
    map.insert(driverNameKeyC, name);
    return map;
}

void DriverSelection::fromMap(const Store &map)
{
    index = map.value(driverIndexKeyC).toInt();
    cpuDllIndex = map.value(driverCpuDllIndexKeyC).toInt();
    dll = map.value(driverDllKeyC).toString();
    cpuDlls = map.value(driverCpuDllsKeyC).toStringList();
    name = map.value(driverNameKeyC).toString();
}

bool DriverSelection::operator==(const DriverSelection &other) const
{
    return index == other.index && cpuDllIndex == other.cpuDllIndex
            && dll == other.dll && cpuDlls == other.cpuDlls && name == other.name;
}

// DriverSelectionCpuDllItem

class DriverSelectionCpuDllItem final : public TreeItem
{
public:
    enum Column { DllNameColumn };
    explicit DriverSelectionCpuDllItem(int index, DriverSelection &selection)
        : m_index(index), m_selection(selection)
    {}

    QVariant data(int column, int role) const final
    {
        if (role == Qt::DisplayRole) {
            const auto &dll = m_selection.cpuDlls.at(m_index);
            switch (column) {
            case DllNameColumn: return dll;
            }
        }
        return {};
    }

private:
    const int m_index;
    DriverSelection &m_selection;
};

// DriverSelectionCpuDllModel

DriverSelectionCpuDllModel::DriverSelectionCpuDllModel(DriverSelection &selection, QObject *parent)
    : TreeModel<TreeItem, DriverSelectionCpuDllItem>(parent), m_selection(selection)
{
    setHeader({Tr::tr("Name")});
    refresh();
}

void DriverSelectionCpuDllModel::refresh()
{
    clear();

    const auto begin = m_selection.cpuDlls.begin();
    for (auto it = begin; it < m_selection.cpuDlls.end(); ++it) {
        const auto index = std::distance(begin, it);
        const auto item = new DriverSelectionCpuDllItem(index, m_selection);
        rootItem()->appendChild(item);
    }
}

// DriverSelectionCpuDllView

DriverSelectionCpuDllView::DriverSelectionCpuDllView(DriverSelection &selection, QWidget *parent)
    : QWidget(parent)
{
    const auto model = new DriverSelectionCpuDllModel(selection, this);
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    m_comboBox = new QComboBox;
    m_comboBox->setToolTip(Tr::tr("Debugger CPU library (depends on a CPU core)."));
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBox->setModel(model);
    layout->addWidget(m_comboBox);
    setLayout(layout);

    connect(m_comboBox, &QComboBox::currentIndexChanged,
            this, &DriverSelectionCpuDllView::dllChanged);
}

void DriverSelectionCpuDllView::setCpuDll(int index)
{
    m_comboBox->setCurrentIndex(index);
}

void DriverSelectionCpuDllView::refresh()
{
    const QSignalBlocker blocker(this);
    qobject_cast<DriverSelectionCpuDllModel *>(m_comboBox->model())->refresh();
}

} // BareMetal::Internal::Uv
