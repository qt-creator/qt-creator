/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

DeploymentDataView::DeploymentDataView(Target *target, QWidget *parent) :
    NamedWidget(parent), d(new DeploymentDataViewPrivate)
{
    d->ui.setupUi(this);
    d->ui.deploymentDataView->setTextElideMode(Qt::ElideMiddle);
    d->ui.deploymentDataView->setWordWrap(false);
    d->ui.deploymentDataView->setUniformRowHeights(true);
    d->ui.deploymentDataView->setModel(&d->deploymentDataModel);

    d->target = target;

    connect(target, SIGNAL(deploymentDataChanged()), SLOT(updateDeploymentDataModel()));
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
