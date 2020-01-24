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

#pragma once

#include <utils/basetreeview.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {
namespace Uv {

// DriverSelection

class DriverSelection final
{
public:
    QString name;
    QString dll;
    QStringList cpuDlls;
    int index = 0;
    int cpuDllIndex = 0;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    bool operator==(const DriverSelection &other) const;
};

// DriverSelectionCpuDllModel

class DriverSelectionCpuDllItem;
class DriverSelectionCpuDllModel final
        : public Utils::TreeModel<Utils::TreeItem, DriverSelectionCpuDllItem>
{
    Q_OBJECT

public:
    explicit DriverSelectionCpuDllModel(DriverSelection &selection, QObject *parent = nullptr);
    void refresh();

private:
    DriverSelection &m_selection;
};

// DriverSelectionCpuDllView

class DriverSelectionCpuDllView final : public QWidget
{
    Q_OBJECT

public:
    explicit DriverSelectionCpuDllView(DriverSelection &selection, QWidget *parent = nullptr);
    void setCpuDll(int index);
    void refresh();

signals:
    void dllChanged(int index = -1);

private:
    QComboBox *m_comboBox = nullptr;
};

} // namespace Uv
} // namespace Internal
} // namespace BareMetal

Q_DECLARE_METATYPE(BareMetal::Internal::Uv::DriverSelection)
