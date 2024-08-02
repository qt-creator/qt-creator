// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "cameraviewwidgetaction.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QComboBox>

namespace QmlDesigner {

struct CameraActionsModel::DataItem
{
    QString name;
    QString tooltip;
    QString mode;
};

const QList<CameraActionsModel::DataItem> CameraActionsModel::m_data{
    {tr("Hide Camera View"), tr("Never show the camera view."), "CameraOff"},
    {tr("Show Selected Camera View"),
     tr("Show the selected camera in the camera view."),
     "ShowSelectedCamera"},
    {tr("Always Show Camera View"),
     tr("Show the last selected camera in the camera view."),
     "AlwaysShowCamera"},
};

CameraViewWidgetAction::CameraViewWidgetAction(QObject *parent)
    : QWidgetAction(parent)
{
    QComboBox *defaultComboBox = new QComboBox();
    CameraActionsModel *comboBoxModel = new CameraActionsModel(defaultComboBox);

    defaultComboBox->setModel(comboBoxModel);
    setDefaultWidget(defaultComboBox);
    connect(defaultComboBox, &QComboBox::currentIndexChanged, this, [this] {
        emit currentModeChanged(currentMode());
    });
}

QString CameraViewWidgetAction::currentMode() const
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return "CameraOff");

    return defaultComboBox->currentData(CameraActionsModel::ModeRole).toString();
}

void CameraViewWidgetAction::setMode(const QString &mode)
{
    QComboBox *defaultComboBox = qobject_cast<QComboBox *>(defaultWidget());
    QTC_ASSERT(defaultComboBox, return);
    defaultComboBox->setCurrentIndex(CameraActionsModel::modeIndex(mode));
}

QWidget *CameraViewWidgetAction::createWidget(QWidget *parent)
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

CameraActionsModel::CameraActionsModel(QObject *parent)
    : QAbstractListModel(parent)
{}

CameraActionsModel::~CameraActionsModel() = default;

int CameraActionsModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_data.size();
}

QVariant CameraActionsModel::data(const QModelIndex &index, int role) const
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

int CameraActionsModel::modeIndex(const QString &mode)
{
    int idx = Utils::indexOf(m_data, Utils::equal(&DataItem::mode, mode));
    return std::max(0, idx);
}

} // namespace QmlDesigner
