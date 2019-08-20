/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "deploymentdataview.h"

#include "deploymentdata.h"
#include "target.h"

#include <utils/treemodel.h>

#include <QAbstractTableModel>
#include <QLabel>
#include <QHeaderView>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class DeploymentDataItem : public TreeItem
{
public:
    DeploymentDataItem() = default;
    DeploymentDataItem(const DeployableFile &file) : file(file) {}

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole)
            return column == 0 ? file.localFilePath().toUserOutput() : file.remoteDirectory();
        return QVariant();
    }
    DeployableFile file;
};


DeploymentDataView::DeploymentDataView(Target *target)
{
    auto model = new TreeModel<DeploymentDataItem>(this);
    model->setHeader({tr("Local File Path"), tr("Remote Directory")});

    auto view = new QTreeView(this);
    view->setMinimumSize(QSize(100, 100));
    view->setTextElideMode(Qt::ElideMiddle);
    view->setWordWrap(false);
    view->setUniformRowHeights(true);
    view->setModel(model);

    auto label = new QLabel(tr("Files to deploy:"), this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
    layout->addWidget(view);

    auto updatModel = [this, target, model, view] {
        model->clear();
        for (const DeployableFile &file : target->deploymentData().allFiles())
            model->rootItem()->appendChild(new DeploymentDataItem(file));

        QHeaderView *header = view->header();
        header->setSectionResizeMode(0, QHeaderView::Interactive);
        header->setSectionResizeMode(1, QHeaderView::Interactive);
        view->resizeColumnToContents(0);
        view->resizeColumnToContents(1);
        if (header->sectionSize(0) + header->sectionSize(1) < header->width())
            header->setSectionResizeMode(1, QHeaderView::Stretch);
    };

    connect(target, &Target::deploymentDataChanged, this, updatModel);
    updatModel();
}

} // Internal
} // ProjectExplorer
