// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "comboboxaction.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QComboBox>

namespace QmlDesigner {

ComboBoxAction::ComboBoxAction(const QList<ComboBoxActionsModel::DataItem> &dataItems, QObject *parent)
    : QWidgetAction(parent)
{
    QComboBox *defaultComboBox = new QComboBox();
    m_model = new ComboBoxActionsModel(dataItems, defaultComboBox);

    defaultComboBox->setModel(m_model);
    setDefaultWidget(defaultComboBox);
    connect(defaultComboBox, &QComboBox::currentIndexChanged, this, [this] {
        emit currentModeChanged(currentMode());
    });
}

QString ComboBoxAction::currentMode() const
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return m_model->defaultMode());

    return defaultComboBox->currentData(ComboBoxActionsModel::ModeRole).toString();
}

void ComboBoxAction::setMode(const QString &mode)
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return);
    int i = m_model->modeIndex(mode);
    if (defaultComboBox->currentIndex() != i)
        defaultComboBox->setCurrentIndex(i);
}

void ComboBoxAction::cycleMode()
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return);
    int i = defaultComboBox->currentIndex();
    if (++i >= defaultComboBox->count())
        i = 0;
    defaultComboBox->setCurrentIndex(i);
}

QWidget *ComboBoxAction::createWidget(QWidget *parent)
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return nullptr);

    QComboBox *newComboBox = new QComboBox(parent);
    newComboBox->setModel(defaultComboBox->model());
    connect(defaultComboBox, &QComboBox::currentIndexChanged, newComboBox, &QComboBox::setCurrentIndex);
    connect(newComboBox, &QComboBox::currentIndexChanged, defaultComboBox, &QComboBox::setCurrentIndex);
    newComboBox->setCurrentIndex(defaultComboBox->currentIndex());
    return newComboBox;
}

ComboBoxActionsModel::ComboBoxActionsModel(const QList<ComboBoxActionsModel::DataItem> &dataItems,
                                           QObject *parent)
    : QAbstractListModel(parent)
    , m_data(dataItems)
{

}

ComboBoxActionsModel::~ComboBoxActionsModel() = default;

int ComboBoxActionsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_data.size();
}

QVariant ComboBoxActionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const DataItem &data = m_data.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.name;
    case Qt::ToolTipRole:
        return data.tooltip;
    case ExtendedDataRoles::ModeRole:
        return data.mode;
    default:
        return {};
    }
}

int ComboBoxActionsModel::modeIndex(const QString &mode)
{
    int idx = Utils::indexOf(m_data, Utils::equal(&DataItem::mode, mode));
    return std::max(0, idx);
}

QString ComboBoxActionsModel::defaultMode() const
{
    if (!m_data.isEmpty())
        return m_data.at(0).mode;
    return {};
}

} // namespace QmlDesigner
