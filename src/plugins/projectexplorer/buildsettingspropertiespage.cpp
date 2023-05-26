// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsettingspropertiespage.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "namedwidget.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stringutils.h>

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
using namespace Utils;

///
// BuildSettingsWidget
///

BuildSettingsWidget::~BuildSettingsWidget()
{
    clearWidgets();
}

BuildSettingsWidget::BuildSettingsWidget(Target *target) :
    m_target(target)
{
    Q_ASSERT(m_target);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    if (!BuildConfigurationFactory::find(m_target)) {
        auto noSettingsLabel = new QLabel(this);
        noSettingsLabel->setText(Tr::tr("No build settings available"));
        QFont f = noSettingsLabel->font();
        f.setPointSizeF(f.pointSizeF() * 1.2);
        noSettingsLabel->setFont(f);
        vbox->addWidget(noSettingsLabel);
        return;
    }

    { // Edit Build Configuration row
        auto hbox = new QHBoxLayout();
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(new QLabel(Tr::tr("Edit build configuration:"), this));
        m_buildConfigurationComboBox = new QComboBox(this);
        m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_buildConfigurationComboBox->setModel(m_target->buildConfigurationModel());
        hbox->addWidget(m_buildConfigurationComboBox);

        m_addButton = new QPushButton(this);
        m_addButton->setText(Tr::tr("Add"));
        m_addButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_addButton);
        m_addButtonMenu = new QMenu(this);
        m_addButton->setMenu(m_addButtonMenu);

        m_removeButton = new QPushButton(this);
        m_removeButton->setText(Tr::tr("Remove"));
        m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_removeButton);

        m_renameButton = new QPushButton(this);
        m_renameButton->setText(Tr::tr("Rename..."));
        m_renameButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_renameButton);

        m_cloneButton = new QPushButton(this);
        m_cloneButton->setText(Tr::tr("Clone..."));
        m_cloneButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        hbox->addWidget(m_cloneButton);

        hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
        vbox->addLayout(hbox);
    }

    m_buildConfiguration = m_target->activeBuildConfiguration();
    m_buildConfigurationComboBox->setCurrentIndex(
                m_target->buildConfigurationModel()->indexFor(m_buildConfiguration));

    updateAddButtonMenu();
    updateBuildSettings();

    connect(m_buildConfigurationComboBox, &QComboBox::currentIndexChanged,
            this, &BuildSettingsWidget::currentIndexChanged);

    connect(m_removeButton, &QAbstractButton::clicked,
            this, [this] { deleteConfiguration(m_buildConfiguration); });

    connect(m_renameButton, &QAbstractButton::clicked,
            this, &BuildSettingsWidget::renameConfiguration);

    connect(m_cloneButton, &QAbstractButton::clicked,
            this, &BuildSettingsWidget::cloneConfiguration);

    connect(m_target, &Target::activeBuildConfigurationChanged,
            this, &BuildSettingsWidget::updateActiveConfiguration);

    connect(m_target, &Target::kitChanged, this, &BuildSettingsWidget::updateAddButtonMenu);
}

void BuildSettingsWidget::addSubWidget(NamedWidget *widget)
{
    widget->setParent(this);
    widget->setContentsMargins(0, 2, 0, 0);

    auto label = new QLabel(this);
    label->setText(widget->displayName());
    QFont f = label->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    label->setFont(f);

    label->setContentsMargins(0, 18, 0, 0);

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

void BuildSettingsWidget::updateAddButtonMenu()
{
    m_addButtonMenu->clear();

    if (m_target) {
        BuildConfigurationFactory *factory = BuildConfigurationFactory::find(m_target);
        if (!factory)
            return;
        for (const BuildInfo &info : factory->allAvailableBuilds(m_target)) {
            QAction *action = m_addButtonMenu->addAction(info.typeName);
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
    QList<BuildConfiguration *> bcs = m_target->buildConfigurations();
    m_removeButton->setEnabled(bcs.size() > 1);
    m_renameButton->setEnabled(!bcs.isEmpty());
    m_cloneButton->setEnabled(!bcs.isEmpty());

    if (m_buildConfiguration)
        m_buildConfiguration->addConfigWidgets([this](NamedWidget *w) { addSubWidget(w); });
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    auto buildConfiguration = qobject_cast<BuildConfiguration *>(
                m_target->buildConfigurationModel()->projectConfigurationAt(index));
    m_target->setActiveBuildConfiguration(buildConfiguration, SetActive::Cascade);
}

void BuildSettingsWidget::updateActiveConfiguration()
{
    if (!m_buildConfiguration || m_buildConfiguration == m_target->activeBuildConfiguration())
        return;

    m_buildConfiguration = m_target->activeBuildConfiguration();

    m_buildConfigurationComboBox->setCurrentIndex(
                m_target->buildConfigurationModel()->indexFor(m_buildConfiguration));

    updateBuildSettings();
}

void BuildSettingsWidget::createConfiguration(const BuildInfo &info_)
{
    BuildInfo info = info_;
    if (info.displayName.isEmpty()) {
        bool ok = false;
        info.displayName = QInputDialog::getText(Core::ICore::dialogParent(),
                                                 Tr::tr("New Configuration"),
                                                 Tr::tr("New configuration name:"),
                                                 QLineEdit::Normal,
                                                 QString(),
                                                 &ok)
                               .trimmed();
        if (!ok || info.displayName.isEmpty())
            return;
    }

    BuildConfiguration *bc = info.factory->create(m_target, info);
    if (!bc)
        return;

    m_target->addBuildConfiguration(bc);
    m_target->setActiveBuildConfiguration(bc, SetActive::Cascade);
}

QString BuildSettingsWidget::uniqueName(const QString & name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList bcNames;
        for (BuildConfiguration *bc : m_target->buildConfigurations()) {
            if (bc == m_buildConfiguration)
                continue;
            bcNames.append(bc->displayName());
        }
        result = makeUniquelyNumbered(result, bcNames);
    }
    return result;
}

void BuildSettingsWidget::renameConfiguration()
{
    QTC_ASSERT(m_buildConfiguration, return);
    bool ok;
    QString name = QInputDialog::getText(this, Tr::tr("Rename..."),
                                         Tr::tr("New name for build configuration <b>%1</b>:").
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

void BuildSettingsWidget::cloneConfiguration()
{
    QTC_ASSERT(m_buildConfiguration, return);
    BuildConfigurationFactory *factory = BuildConfigurationFactory::find(m_target);
    if (!factory)
        return;

    //: Title of a the cloned BuildConfiguration window, text of the window
    QString name = uniqueName(QInputDialog::getText(this,
                                                    Tr::tr("Clone Configuration"),
                                                    Tr::tr("New configuration name:"),
                                                    QLineEdit::Normal,
                                                    m_buildConfiguration->displayName()));
    if (name.isEmpty())
        return;

    // Save the current build configuration settings, so that the clone gets all the settings
    m_target->project()->saveSettings();

    BuildConfiguration *bc = BuildConfigurationFactory::clone(m_target, m_buildConfiguration);
    if (!bc)
        return;

    bc->setDisplayName(name);
    const FilePath buildDirectory = bc->buildDirectory();
    if (buildDirectory != m_target->project()->projectDirectory()) {
        const FilePathPredicate isBuildDirOk = [this](const FilePath &candidate) {
            if (candidate.exists())
                return false;
            return !anyOf(m_target->buildConfigurations(), [&candidate](const BuildConfiguration *bc) {
                return bc->buildDirectory() == candidate; });
        };
        bc->setBuildDirectory(makeUniquelyNumbered(buildDirectory, isBuildDirOk));
    }
    m_target->addBuildConfiguration(bc);
    m_target->setActiveBuildConfiguration(bc, SetActive::Cascade);
}

void BuildSettingsWidget::deleteConfiguration(BuildConfiguration *deleteConfiguration)
{
    if (!deleteConfiguration ||
        m_target->buildConfigurations().size() <= 1)
        return;

    if (BuildManager::isBuilding(deleteConfiguration)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(Tr::tr("Cancel Build && Remove Build Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(Tr::tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(Tr::tr("Remove Build Configuration %1?").arg(deleteConfiguration->displayName()));
        box.setText(Tr::tr("The build configuration <b>%1</b> is currently being built.").arg(deleteConfiguration->displayName()));
        box.setInformativeText(Tr::tr("Do you want to cancel the build process and remove the Build Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, Tr::tr("Remove Build Configuration?"),
                           Tr::tr("Do you really want to delete build configuration <b>%1</b>?").arg(deleteConfiguration->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    m_target->removeBuildConfiguration(deleteConfiguration);
}
