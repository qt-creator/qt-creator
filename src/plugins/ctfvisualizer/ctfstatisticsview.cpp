/****************************************************************************
**
** Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
** info@kdab.com, author Tim Henning <tim.henning@kdab.com>
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
#include "ctfstatisticsview.h"

#include "ctfstatisticsmodel.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>

namespace CtfVisualizer {
namespace Internal {

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
    setUniformRowHeights(true);
    setSortingEnabled(true);

    connect(selectionModel(), &QItemSelectionModel::currentChanged,
            [this] (const QModelIndex &current, const QModelIndex &previous)
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

}  // Internal
}  // CtfVisualizer
