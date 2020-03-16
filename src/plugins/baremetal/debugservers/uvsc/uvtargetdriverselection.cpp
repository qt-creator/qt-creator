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

#include "uvtargetdriverselection.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

using namespace Utils;

namespace BareMetal {
namespace Internal {
namespace Uv {

// Driver data keys.
constexpr char driverIndexKeyC[] = "BareMetal.UvscServerProvider.DriverIndex";
constexpr char driverCpuDllIndexKeyC[] = "BareMetal.UvscServerProvider.DriverCpuDllIndex";
constexpr char driverDllKeyC[] = "BareMetal.UvscServerProvider.DriverDll";
constexpr char driverCpuDllsKeyC[] = "BareMetal.UvscServerProvider.DriverCpuDlls";
constexpr char driverNameKeyC[] = "BareMetal.UvscServerProvider.DriverName";

// DriverSelection

QVariantMap DriverSelection::toMap() const
{
    QVariantMap map;
    map.insert(driverIndexKeyC, index);
    map.insert(driverCpuDllIndexKeyC, cpuDllIndex);
    map.insert(driverDllKeyC, dll);
    map.insert(driverCpuDllsKeyC, cpuDlls);
    map.insert(driverNameKeyC, name);
    return map;
}

void DriverSelection::fromMap(const QVariantMap &map)
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
    setHeader({tr("Name")});
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
    m_comboBox->setToolTip(tr("Debugger CPU library (depends on a CPU core)."));
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBox->setModel(model);
    layout->addWidget(m_comboBox);
    setLayout(layout);

    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
