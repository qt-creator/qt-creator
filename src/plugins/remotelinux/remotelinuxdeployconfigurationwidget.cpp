/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "remotelinuxdeployconfigurationwidget.h"
#include "ui_remotelinuxdeployconfigurationwidget.h"

#include "deployablefilesperprofile.h"
#include "deploymentinfo.h"
#include "remotelinuxdeployconfiguration.h"
#include "typespecificdeviceconfigurationlistmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QTreeView>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {
namespace {
class MyTreeView : public QTreeView
{
    Q_OBJECT
public:
    MyTreeView(QWidget *parent = 0) : QTreeView(parent) {}

signals:
    void doubleClicked();

private:
    void mouseDoubleClickEvent(QMouseEvent *event)
    {
        emit doubleClicked();
        QTreeView::mouseDoubleClickEvent(event);
    }
};

} // anonymous namespace

class RemoteLinuxDeployConfigurationWidgetPrivate
{
public:
    Ui::RemoteLinuxDeployConfigurationWidget ui;
    MyTreeView treeView;
    RemoteLinuxDeployConfiguration *deployConfiguration;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxDeployConfigurationWidget::RemoteLinuxDeployConfigurationWidget(QWidget *parent) :
    DeployConfigurationWidget(parent), d(new RemoteLinuxDeployConfigurationWidgetPrivate)
{
    d->ui.setupUi(this);
    d->treeView.setTextElideMode(Qt::ElideMiddle);
    d->treeView.setWordWrap(false);
    d->treeView.setUniformRowHeights(true);
    layout()->addWidget(&d->treeView);
}

RemoteLinuxDeployConfigurationWidget::~RemoteLinuxDeployConfigurationWidget()
{
    delete d;
}

void RemoteLinuxDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    d->deployConfiguration = qobject_cast<RemoteLinuxDeployConfiguration *>(dc);
    Q_ASSERT(d->deployConfiguration);

    connect(&d->treeView, SIGNAL(doubleClicked()), SLOT(openProjectFile()));

    d->ui.projectsComboBox->setModel(d->deployConfiguration->deploymentInfo());
    connect(d->deployConfiguration->deploymentInfo(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(d->deployConfiguration->deploymentInfo(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(d->ui.projectsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModel(int)));
    handleModelListReset();
}

RemoteLinuxDeployConfiguration *RemoteLinuxDeployConfigurationWidget::deployConfiguration() const
{
    return d->deployConfiguration;
}

DeployableFilesPerProFile *RemoteLinuxDeployConfigurationWidget::currentModel() const
{
    const int modelRow = d->ui.projectsComboBox->currentIndex();
    if (modelRow == -1)
        return 0;
    return d->deployConfiguration->deploymentInfo()->modelAt(modelRow);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListToBeReset()
{
    d->treeView.setModel(0);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListReset()
{
    QTC_ASSERT(d->deployConfiguration->deploymentInfo()->modelCount()
        == d->ui.projectsComboBox->count(), return);

    if (d->deployConfiguration->deploymentInfo()->modelCount() > 0) {
        d->treeView.setToolTip(tr("Double-click to edit the project file"));
        if (d->ui.projectsComboBox->currentIndex() == -1)
            d->ui.projectsComboBox->setCurrentIndex(0);
        else
            setModel(d->ui.projectsComboBox->currentIndex());
    } else {
        d->treeView.setToolTip(QString());
    }
}

void RemoteLinuxDeployConfigurationWidget::setModel(int row)
{
    DeployableFilesPerProFile * const proFileInfo = row == -1
        ? 0 : d->deployConfiguration->deploymentInfo()->modelAt(row);
    d->treeView.setModel(proFileInfo);
    if (proFileInfo)
        d->treeView.resizeColumnToContents(0);
    emit currentModelChanged(proFileInfo);
}

void RemoteLinuxDeployConfigurationWidget::openProjectFile()
{
    const int row = d->ui.projectsComboBox->currentIndex();
    if (row == -1)
        return;
    const DeployableFilesPerProFile * const proFileInfo =
        d->deployConfiguration->deploymentInfo()->modelAt(row);
    Core::EditorManager::openEditor(proFileInfo->proFilePath(), Core::Id(),
        Core::EditorManager::ModeSwitch);
}

} // namespace RemoteLinux

#include "remotelinuxdeployconfigurationwidget.moc"
