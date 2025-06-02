// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsettingspropertiespage.h"

#include "buildconfiguration.h"
#include "buildinfo.h"
#include "buildmanager.h"
#include "panelswidget.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/icore.h>
#include <coreplugin/session.h>

#include <utils/algorithm.h>
#include <utils/guiutils.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>

#include <QMargins>
#include <QCoreApplication>
#include <QComboBox>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer::Internal {

class BuildSettingsWidget final : public QWidget
{
public:
    explicit BuildSettingsWidget(Target *target);
    ~BuildSettingsWidget() = default;

private:
    void clearWidgets();
    void addSubWidget(QWidget *widget, const QString &displayName);

    void updateBuildSettings();
    void currentIndexChanged(int index);

    void renameConfiguration();
    void updateAddButtonMenu();

    void updateActiveConfiguration();

    void createConfiguration(const BuildInfo &info);
    void cloneConfiguration();
    void deleteConfiguration(BuildConfiguration *toDelete);
    QString uniqueName(const QString &name, bool allowCurrentName);

    QPointer<Target> m_target;
    QPointer<BuildConfiguration> m_buildConfiguration;

    QPushButton *m_addButton = nullptr;
    QPushButton *m_removeButton = nullptr;
    QPushButton *m_renameButton = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QComboBox *m_buildConfigurationComboBox = nullptr;
    QMenu *m_addButtonMenu = nullptr;

    QList<QWidget *> m_subWidgets;
};

BuildSettingsWidget::BuildSettingsWidget(Target *target)
    : m_target(target)
{
    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    if (!BuildConfigurationFactory::find(m_target)) {
        auto noSettingsLabel = new QLabel(this);
        noSettingsLabel->setText(Tr::tr("No build settings available"));
        noSettingsLabel->setFont(StyleHelper::uiFont(StyleHelper::UiElementH4));
        vbox->addWidget(noSettingsLabel);
        return;
    }

    { // Edit Build Configuration row
        m_buildConfigurationComboBox = new QComboBox(this);
        m_buildConfigurationComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_buildConfigurationComboBox->setModel(m_target->buildConfigurationModel());
        setWheelScrollingWithoutFocusBlocked(m_buildConfigurationComboBox);

        m_addButton = new QPushButton(Tr::tr("Add"), this);
        m_addButtonMenu = new QMenu(this);
        m_addButton->setMenu(m_addButtonMenu);

        m_removeButton = new QPushButton(Tr::tr("Remove"), this);
        m_renameButton = new QPushButton(Tr::tr("Rename..."), this);
        m_cloneButton = new QPushButton(Tr::tr("Clone..."), this);

        auto hbox = new QHBoxLayout();
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(new QLabel(Tr::tr("Edit build configuration:"), this));
        hbox->addWidget(m_buildConfigurationComboBox);
        hbox->addWidget(m_addButton);
        hbox->addWidget(m_removeButton);
        hbox->addWidget(m_renameButton);
        hbox->addWidget(m_cloneButton);
        hbox->addStretch();
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

void BuildSettingsWidget::addSubWidget(QWidget *widget, const QString &displayName)
{
    widget->setParent(this);
    widget->setContentsMargins(0, 2, 0, 0);

    auto label = new QLabel(this);
    label->setText(displayName);
    label->setFont(StyleHelper::uiFont(StyleHelper::UiElementH4));
    label->setContentsMargins(0, 18, 0, 0);

    layout()->addWidget(label);
    layout()->addWidget(widget);

    m_subWidgets.append(label);
    m_subWidgets.append(widget);
}

void BuildSettingsWidget::clearWidgets()
{
    qDeleteAll(m_subWidgets);
    m_subWidgets.clear();
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

    if (m_buildConfiguration) {
        m_buildConfiguration->addConfigWidgets([this](QWidget *w, const QString &displayName) {
            addSubWidget(w, displayName);
        });
    }
}

void BuildSettingsWidget::currentIndexChanged(int index)
{
    auto buildConfiguration = qobject_cast<BuildConfiguration *>(
                m_target->buildConfigurationModel()->projectConfigurationAt(index));
    m_target->setActiveBuildConfiguration(buildConfiguration, SetActive::Cascade);
}

void BuildSettingsWidget::updateActiveConfiguration()
{
    if (m_buildConfiguration == m_target->activeBuildConfiguration())
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
        info.displayName = uniqueName(
                               QInputDialog::getText(
                                   Core::ICore::dialogParent(),
                                   Tr::tr("New Configuration"),
                                   Tr::tr("New configuration name:"),
                                   QLineEdit::Normal,
                                   QString(),
                                   &ok),
                               false)
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

QString BuildSettingsWidget::uniqueName(const QString &name, bool allowCurrentName)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList bcNames;
        for (BuildConfiguration *bc : m_target->buildConfigurations()) {
            if (allowCurrentName && bc == m_buildConfiguration)
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

    name = uniqueName(name, true);
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
    QString name = uniqueName(
        QInputDialog::getText(
            this,
            Tr::tr("Clone Configuration"),
            Tr::tr("New configuration name:"),
            QLineEdit::Normal,
            m_buildConfiguration->displayName()),
        false);
    if (name.isEmpty())
        return;

    // Save the current build configuration settings, so that the clone gets all the settings
    m_buildConfiguration->project()->saveSettings();

    BuildConfiguration *bc = m_buildConfiguration->clone(m_target);
    if (!bc)
        return;

    bc->setDisplayName(name);
    const FilePath buildDirectory = BuildConfiguration::buildDirectoryFromTemplate(
        bc->project()->projectDirectory(),
        bc->project()->projectFilePath(),
        bc->project()->displayName(),
        bc->kit(),
        name,
        bc->buildType(),
        bc->project()->buildSystemName());
    bc->setBuildDirectory(buildDirectory);
    if (buildDirectory != bc->project()->projectDirectory()) {
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

QWidget *createBuildSettingsWidget(Target *target)
{
    auto panel = new PanelsWidget(true);
    panel->addWidget(new BuildSettingsWidget(target));
    return panel;
}

} // ProjectExplorer::Internal
