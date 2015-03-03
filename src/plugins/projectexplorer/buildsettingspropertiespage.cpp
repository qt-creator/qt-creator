/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "buildsettingspropertiespage.h"
#include "buildinfo.h"
#include "buildstepspage.h"
#include "project.h"
#include "target.h"
#include "buildconfiguration.h"
#include "buildconfigurationmodel.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>

#include <QMargins>
#include <QCoreApplication>
#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

///
// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
    clearWidgets();
    qDeleteAll(m_buildInfoList);
}

BuildSettingsWidget::BuildSettingsWidget(Target *target) :
    m_target(target),
    m_buildConfiguration(0)
{
    Q_ASSERT(m_target);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    if (!IBuildConfigurationFactory::find(m_target)) {
        QLabel *noSettingsLabel = new QLabel(this);
        noSettingsLabel->setText(tr("No build settings available"));
        QFont f = noSettingsLabel->font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        noSettingsLabel->setFont(f);
        vbox->addWidget(noSettingsLabel);
        return;
    }

    { // Edit Build Configuration row
        QHBoxLayout *hbox = new QHBoxLayout();
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(new QLabel(tr("Edit build configuration:"), this));
        m_buildConfigurationComboBox = new QComboBox(this);
        m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_buildConfigurationComboBox->setModel(new BuildConfigurationModel(m_target, this));
        hbox->addWidget(m_buildConfigurationComboBox);

        m_addButton = new QPushButton(this);
        m_addButton->setText(tr("Add"));
        m_addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_addButton);
        m_addButtonMenu = new QMenu(this);
        m_addButton->setMenu(m_addButtonMenu);

        m_removeButton = new QPushButton(this);
        m_removeButton->setText(tr("Remove"));
        m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_removeButton);

        m_renameButton = new QPushButton(this);
        m_renameButton->setText(tr("Rename..."));
        m_renameButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_renameButton);

        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        vbox->addLayout(hbox);
    }

    m_buildConfiguration = m_target->activeBuildConfiguration();
    BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildConfigurationComboBox->model());
    m_buildConfigurationComboBox->setCurrentIndex(model->indexFor(m_buildConfiguration).row());

    updateAddButtonMenu();
    updateBuildSettings();

    connect(m_buildConfigurationComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(currentIndexChanged(int)));

    connect(m_removeButton, SIGNAL(clicked()),
            this, SLOT(deleteConfiguration()));

    connect(m_renameButton, SIGNAL(clicked()),
            this, SLOT(renameConfiguration()));

    connect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(updateActiveConfiguration()));

    connect(m_target, SIGNAL(kitChanged()), this, SLOT(updateAddButtonMenu()));
}

void BuildSettingsWidget::addSubWidget(NamedWidget *widget)
{
    widget->setContentsMargins(0, 10, 0, 0);

    QLabel *label = new QLabel(this);
    label->setText(widget->displayName());
    connect(widget, SIGNAL(displayNameChanged(QString)),
            label, SLOT(setText(QString)));
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 10, 0, 0);

    layout()->addWidget(label);
    layout()->addWidget(widget);

    m_labels.append(label);
    m_subWidgets.append(widget);
}

void BuildSettingsWidget::clearWidgets()
{
    qDeleteAll(m_subWidgets);
    m_subWidgets.clear();
    qDeleteAll(m_labels);
    m_labels.clear();
}

QList<NamedWidget *> BuildSettingsWidget::subWidgets() const
{
    return m_subWidgets;
}

void BuildSettingsWidget::updateAddButtonMenu()
{
    m_addButtonMenu->clear();
    qDeleteAll(m_buildInfoList);
    m_buildInfoList.clear();

    if (m_target) {
        if (m_target->activeBuildConfiguration()) {
            m_addButtonMenu->addAction(tr("&Clone Selected"),
                                       this, SLOT(cloneConfiguration()));
        }
        IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(m_target);
        if (!factory)
            return;
        m_buildInfoList = factory->availableBuilds(m_target);
        foreach (BuildInfo *info, m_buildInfoList) {
            QAction *action = m_addButtonMenu->addAction(info->typeName);
            connect(action, &QAction::triggered, this, [this, info] {
                createConfiguration(info);
            });
        }
    }
}

void BuildSettingsWidget::updateBuildSettings()
{
    clearWidgets();

    // update buttons
    m_removeButton->setEnabled(m_target->buildConfigurations().size() > 1);

    if (!m_buildConfiguration)
        return;

    // Add pages
    NamedWidget *generalConfigWidget = m_buildConfiguration->createConfigWidget();
    if (generalConfigWidget)
        addSubWidget(generalConfigWidget);

    addSubWidget(new BuildStepsPage(m_buildConfiguration, Core::Id(Constants::BUILDSTEPS_BUILD)));
    addSubWidget(new BuildStepsPage(m_buildConfiguration, Core::Id(Constants::BUILDSTEPS_CLEAN)));

    QList<NamedWidget *> subConfigWidgets = m_buildConfiguration->createSubConfigWidgets();
    foreach (NamedWidget *subConfigWidget, subConfigWidgets)
        addSubWidget(subConfigWidget);
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildConfigurationComboBox->model());
    BuildConfiguration *buildConfiguration = model->buildConfigurationAt(index);
    m_target->setActiveBuildConfiguration(buildConfiguration);
}

void BuildSettingsWidget::updateActiveConfiguration()
{
    if (!m_buildConfiguration || m_buildConfiguration == m_target->activeBuildConfiguration())
        return;

    m_buildConfiguration = m_target->activeBuildConfiguration();

    BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildConfigurationComboBox->model());
    m_buildConfigurationComboBox->setCurrentIndex(model->indexFor(m_buildConfiguration).row());

    updateBuildSettings();
}

void BuildSettingsWidget::createConfiguration(BuildInfo *info)
{
    QString originalDisplayName = info->displayName;

    if (info->displayName.isEmpty()) {
        bool ok = false;
        info->displayName = QInputDialog::getText(Core::ICore::mainWindow(),
                                                  tr("New Configuration"),
                                                  tr("New configuration name:"),
                                                  QLineEdit::Normal,
                                                  QString(), &ok).trimmed();
        if (!ok || info->displayName.isEmpty())
            return;
    }

    BuildConfiguration *bc = info->factory()->create(m_target, info);
    if (!bc)
        return;

    m_target->addBuildConfiguration(bc);
    m_target->setActiveBuildConfiguration(bc);
    info->displayName = originalDisplayName;
}

void BuildSettingsWidget::cloneConfiguration()
{
    cloneConfiguration(m_buildConfiguration);
}

void BuildSettingsWidget::deleteConfiguration()
{
    deleteConfiguration(m_buildConfiguration);
}

QString BuildSettingsWidget::uniqueName(const QString & name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList bcNames;
        foreach (BuildConfiguration *bc, m_target->buildConfigurations()) {
            if (bc == m_buildConfiguration)
                continue;
            bcNames.append(bc->displayName());
        }
        result = Project::makeUnique(result, bcNames);
    }
    return result;
}

void BuildSettingsWidget::renameConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Rename..."),
                                         tr("New name for build configuration <b>%1</b>:").
                                            arg(m_buildConfiguration->displayName()),
                                         QLineEdit::Normal,
                                         m_buildConfiguration->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueName(name);
    if (name.isEmpty())
        return;

    m_buildConfiguration->setDisplayName(name);

}

void BuildSettingsWidget::cloneConfiguration(BuildConfiguration *sourceConfiguration)
{
    if (!sourceConfiguration)
        return;
    IBuildConfigurationFactory *factory = IBuildConfigurationFactory::find(m_target);
    if (!factory)
        return;

    //: Title of a the cloned BuildConfiguration window, text of the window
    QString name = uniqueName(QInputDialog::getText(this, tr("Clone Configuration"), tr("New configuration name:")));
    if (name.isEmpty())
        return;

    BuildConfiguration *bc = factory->clone(m_target, sourceConfiguration);
    if (!bc)
        return;

    bc->setDisplayName(name);
    m_target->addBuildConfiguration(bc);
    m_target->setActiveBuildConfiguration(bc);
}

void BuildSettingsWidget::deleteConfiguration(BuildConfiguration *deleteConfiguration)
{
    if (!deleteConfiguration ||
        m_target->buildConfigurations().size() <= 1)
        return;

    if (BuildManager::isBuilding(deleteConfiguration)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(tr("Cancel Build && Remove Build Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(tr("Remove Build Configuration %1?").arg(deleteConfiguration->displayName()));
        box.setText(tr("The build configuration <b>%1</b> is currently being built.").arg(deleteConfiguration->displayName()));
        box.setInformativeText(tr("Do you want to cancel the build process and remove the Build Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, tr("Remove Build Configuration?"),
                           tr("Do you really want to delete build configuration <b>%1</b>?").arg(deleteConfiguration->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    m_target->removeBuildConfiguration(deleteConfiguration);
}
