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
#include "ui_deploymentdataview.h"

#include "deploymentdatamodel.h"
#include "target.h"

namespace ProjectExplorer {
namespace Internal {

class DeploymentDataViewPrivate
{
public:
    Ui::DeploymentDataView ui;
    Target *target;
    DeploymentDataModel deploymentDataModel;
};

} // namespace Internal

using namespace Internal;

DeploymentDataView::DeploymentDataView(Target *target, QWidget *parent) : NamedWidget(parent),
    d(new DeploymentDataViewPrivate)
{
    d->ui.setupUi(this);
    d->ui.deploymentDataView->setTextElideMode(Qt::ElideMiddle);
    d->ui.deploymentDataView->setWordWrap(false);
    d->ui.deploymentDataView->setUniformRowHeights(true);
    d->ui.deploymentDataView->setModel(&d->deploymentDataModel);

    d->target = target;

    connect(target, &Target::deploymentDataChanged,
            this, &DeploymentDataView::updateDeploymentDataModel);
    updateDeploymentDataModel();
}

DeploymentDataView::~DeploymentDataView()
{
    delete d;
}

void DeploymentDataView::updateDeploymentDataModel()
{
    d->deploymentDataModel.setDeploymentData(d->target->deploymentData());
    QHeaderView *header = d->ui.deploymentDataView->header();
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    d->ui.deploymentDataView->resizeColumnToContents(0);
    d->ui.deploymentDataView->resizeColumnToContents(1);
    if (header->sectionSize(0) + header->sectionSize(1)
            < d->ui.deploymentDataView->header()->width()) {
        d->ui.deploymentDataView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    }
}

} // namespace ProjectExplorer
