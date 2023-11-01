// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "ctfstatisticsview.h"

#include "ctfstatisticsmodel.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace CtfVisualizer::Internal {

CtfStatisticsView::CtfStatisticsView(CtfStatisticsModel *model, QWidget *parent)
    : Utils::TreeView(parent)
{
    setObjectName(QLatin1String("CtfVisualizerStatisticsView"));

    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setSourceModel(model);
    sortModel->setSortRole(CtfStatisticsModel::SortRole);
    sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    setModel(sortModel);

    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setDefaultSectionSize(100);
    header()->setMinimumSectionSize(50);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(CtfStatisticsModel::Column::Title, QHeaderView::Stretch);
    setRootIsDecorated(false);
    setSortingEnabled(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current, const QModelIndex &previous)
    {
        Q_UNUSED(previous);
        QModelIndex index = this->model()->index(current.row(), CtfStatisticsModel::Title);
        QString title = this->model()->data(index).toString();
        emit this->eventTypeSelected(title);
    });
}

void CtfStatisticsView::selectByTitle(const QString &title)
{
    auto model = this->model();
    for (int row = 0; row < model->rowCount(); ++row)
    {
        auto index = model->index(row, CtfStatisticsModel::Column::Title);
        if (model->data(index).toString() == title)
        {
            QItemSelection selection(index, model->index(row, CtfStatisticsModel::Column::COUNT - 1));
            selectionModel()->select(selection, QItemSelectionModel::SelectCurrent);
            scrollTo(index);
            break;
        }
    }
}

}  // CtfVisualizer::Internal
