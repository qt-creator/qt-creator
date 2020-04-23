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

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);
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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal

Q_DECLARE_METATYPE(BareMetal::Internal::Uv::DeviceSelection)
