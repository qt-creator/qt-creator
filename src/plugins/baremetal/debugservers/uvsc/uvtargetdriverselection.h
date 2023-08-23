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

// DriverSelection

class DriverSelection final
{
public:
    QString name;
    QString dll;
    QStringList cpuDlls;
    int index = 0;
    int cpuDllIndex = 0;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

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

} // BareMetal::Internal::Uv

Q_DECLARE_METATYPE(BareMetal::Internal::Uv::DriverSelection)
