// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runsettingspropertiespage.h"

#include "buildmanager.h"
#include "buildstepspage.h"
#include "deployconfiguration.h"
#include "project.h"
#include "projectconfigurationmodel.h"
#include "projectexplorerconstants.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "runconfigdialogs.h"
#include "runconfiguration.h"
#include "target.h"

#include <utils/guard.h>
#include <utils/guiutils.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/treemodel.h>

#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QGridLayout>
#include <QHash>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSpacerItem>
#include <QUuid>

using namespace Utils;

namespace ProjectExplorer::Internal {

namespace {
class CloneIntoRunConfigDialog : public QDialog
{
public:
    explicit CloneIntoRunConfigDialog(const RunConfiguration *thisRc)
    {
        setWindowTitle(Tr::tr("Clone From Run Configuration"));
        resize(500, 400);

        // Collect run configurations.
        using RCList = QList<const RunConfiguration *>;
        using RCsPerBuildConfig = QHash<const BuildConfiguration *, RCList>;
        using RCsPerTarget = QHash<const Target *, RCsPerBuildConfig>;
        QHash<const Project *, RCsPerTarget> eligibleRcs;
        const SyncRunConfigs syncSetting
            = ProjectExplorerSettings::get(thisRc).syncRunConfigurations.value();
        const bool considerOtherBuildConfigs = syncSetting == SyncRunConfigs::Off;
        const bool considerOtherTargets = syncSetting != SyncRunConfigs::All;
        for (const Project * const p : ProjectManager::projects()) {
            RCsPerTarget rcsForProject;
            for (const Target * const t : p->targets()) {
                if (p == thisRc->project() && t != thisRc->target() && !considerOtherTargets)
                    continue;
                RCsPerBuildConfig rcsForTarget;
                for (const BuildConfiguration * const bc : t->buildConfigurations()) {
                    if (p == thisRc->project() && bc != thisRc->buildConfiguration()
                        && !considerOtherBuildConfigs) {
                        continue;
                    }
                    RCList rcsForBuildConfig;
                    for (const RunConfiguration * const rc : bc->runConfigurations()) {
                        if (rc != thisRc)
                            rcsForBuildConfig << rc;
                    }
                    if (!rcsForBuildConfig.isEmpty())
                        rcsForTarget.insert(bc, rcsForBuildConfig);
                }
                if (!rcsForTarget.isEmpty())
                    rcsForProject.insert(t, rcsForTarget);
            }
            if (!rcsForProject.isEmpty())
                eligibleRcs.insert(p, rcsForProject);
        }

        // Initialize model. Only use static data. This way, we are immune
        // to removal of any configurations while the dialog is running.
        if (eligibleRcs.isEmpty()) {
            m_rcModel.rootItem()->appendChild(
                new StaticTreeItem(Tr::tr("There are no other run configurations.")));
            m_rcView.setSelectionMode(TreeView::NoSelection);
        } else {
            for (auto projectIt = eligibleRcs.cbegin(); projectIt != eligibleRcs.cend();
                 ++projectIt) {
                const auto projectItem = new StaticTreeItem(projectIt.key()->displayName());
                for (auto targetIt = projectIt.value().cbegin();
                     targetIt != projectIt.value().cend();
                     ++targetIt) {
                    const auto targetItem = new StaticTreeItem(targetIt.key()->displayName());
                    for (auto bcIt = targetIt.value().cbegin(); bcIt != targetIt.value().cend();
                         ++bcIt) {
                        const auto bcItem = new StaticTreeItem(bcIt.key()->displayName());
                        for (const RunConfiguration *const rc : std::as_const(bcIt.value())) {
                            bcItem->appendChild(
                                new RCTreeItem(rc, rc->buildKey() == thisRc->buildKey()));
                        }
                        targetItem->appendChild(bcItem);
                    }
                    projectItem->appendChild(targetItem);
                }
                m_rcModel.rootItem()->appendChild(projectItem);
            }
            m_rcView.setSelectionMode(TreeView::SingleSelection);
            m_rcView.setSelectionBehavior(TreeView::SelectItems);
        }

        // UI
        m_sortModel.setSourceModel(&m_rcModel);

        m_rcView.setModel(&m_sortModel);
        m_rcView.expandAll();
        m_rcView.setSortingEnabled(true);
        m_rcView.resizeColumnToContents(0);
        m_rcView.sortByColumn(0, Qt::AscendingOrder);

        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttonBox->button(QDialogButtonBox::Ok)->setText(Tr::tr("Clone"));

        connect(&m_rcView, &TreeView::doubleClicked, this, [this] { accept(); });

        const auto updateOkButton = [buttonBox, this] {
            buttonBox->button(QDialogButtonBox::Ok)
                ->setEnabled(isRcItem(m_rcView.selectionModel()->currentIndex()));
        };
        connect(m_rcView.selectionModel(), &QItemSelectionModel::selectionChanged,
                this, updateOkButton);
        updateOkButton();

        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        const auto layout = new QVBoxLayout(this);
        layout->addWidget(&m_rcView);
        layout->addWidget(buttonBox);
    }

    const RunConfiguration *source() const { return m_source; }

private:
    class RCTreeItem : public StaticTreeItem
    {
    public:
        RCTreeItem(const RunConfiguration *rc, bool sameBuildKey)
            : StaticTreeItem(rc->displayName()), m_rc(rc), m_sameBuildKey(sameBuildKey) {}
        const RunConfiguration *runConfig() const { return m_rc; }
    private:
        QVariant data(int column, int role) const override
        {
            QTC_ASSERT(column == 0, return {});
            if (role == Qt::FontRole && m_sameBuildKey) {
                QFont f = StaticTreeItem::data(column, role).value<QFont>();
                f.setBold(true);
                return f;
            }
            return StaticTreeItem::data(column, role);
        }

        Qt::ItemFlags flags(int /*column*/) const override
        {
            return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        }

        const QPointer<const RunConfiguration> m_rc;
        const bool m_sameBuildKey;
    };

    void accept() override
    {
        const QModelIndex current = m_rcView.selectionModel()->currentIndex();
        QTC_ASSERT(isRcItem(current), return);
        const auto item = dynamic_cast<RCTreeItem *>(
            m_rcModel.itemForIndex(m_sortModel.mapToSource(current)));
        QTC_ASSERT(item, return);
        m_source = item->runConfig();
        QDialog::accept();
    }

    bool isRcItem(const QModelIndex &index) const
    {
        return index.parent().parent().parent().isValid();
    }

    using RCModel = TreeModel<TreeItem, StaticTreeItem, StaticTreeItem, StaticTreeItem, RCTreeItem>;
    RCModel m_rcModel;
    SortModel m_sortModel;
    TreeView m_rcView;
    const RunConfiguration * m_source = nullptr;
};
} // namespace

// Evil code sharing by inheritance. But restricted to this file.
class RunAndDeploySettingsWidgetBase : public QWidget
{
public:
    RunAndDeploySettingsWidgetBase(Target *target)
        : m_target(target)
    {
        m_subWidgetsLayout = new QVBoxLayout;

        auto topLayout = new QVBoxLayout(this);
        topLayout->setContentsMargins(0, StyleHelper::SpacingTokens::PaddingVXl, 0, 0);
        m_subWidgetsLayout->setSpacing(StyleHelper::SpacingTokens::GapVXxl);
        topLayout->addLayout(m_subWidgetsLayout);
        topLayout->addStretch(1);
    }

protected:
    void addSubWidget(QWidget *widget, QLabel *label)
    {
        label->setFont(StyleHelper::uiFont(StyleHelper::UiElementH4));

        using namespace Layouting;
        QWidget *subWidget = Column {
            label,
            widget,
            noMargin,
        }.emerge();

        m_subWidgetsLayout->addWidget(subWidget);
        m_subWidgets.append(subWidget);
    }

    void removeSubWidgets()
    {
        qDeleteAll(m_subWidgets);
        m_subWidgets.clear();
    }

    QPointer<Target> m_target;
    QWidgetList m_subWidgets;
    QVBoxLayout *m_subWidgetsLayout;
};

// DeploySettingsWidget

class DeploySettingsWidget : public RunAndDeploySettingsWidgetBase
{
public:
    explicit DeploySettingsWidget(Target *target);

private:
    void currentDeployConfigurationChanged(int index);
    void aboutToShowDeployMenu();
    void removeDeployConfiguration();
    void activeDeployConfigurationChanged();
    void renameDeployConfiguration();
    void initForActiveBuildConfig();

    void updateRemoveToolButtons();

    QString uniqueDCName(const QString &name);
    void updateDeployConfiguration(DeployConfiguration *);

    void updateEnabledState();

    QPointer<RunConfiguration> m_runConfiguration;
    QWidget *m_deployConfigurationWidget = nullptr;
    QVBoxLayout *m_deployLayout = nullptr;
    BuildStepListWidget *m_deploySteps = nullptr;
    QMenu *m_addDeployMenu;
    Guard m_ignoreChanges;

    QComboBox *m_deployConfigurationCombo;
    QPushButton *m_addDeployToolButton;
    QPushButton *m_removeDeployToolButton;
    QPushButton *m_renameDeployButton;
    InfoLabel *m_disabledText;
    QObject m_guard;
};

DeploySettingsWidget::DeploySettingsWidget(Target *target)
    : RunAndDeploySettingsWidgetBase(target)
{
    setWindowTitle(Tr::tr("Deploy Settings"));

    m_deployConfigurationCombo = new QComboBox(this);
    setWheelScrollingWithoutFocusBlocked(m_deployConfigurationCombo);

    m_addDeployToolButton = new QPushButton(Tr::tr("Add"), this);
    m_removeDeployToolButton = new QPushButton(Tr::tr("Remove"), this);
    m_renameDeployButton = new QPushButton(Tr::tr("Rename..."), this);

    auto deployWidget = new QWidget(this);

    auto deployLabel = new QLabel(Tr::tr("Active deployment configuration:"), this);
    deployLabel->setBuddy(m_deployConfigurationCombo);

    using namespace Layouting;
    Form actionsLayout {
        noMargin,
        deployLabel,
        Flow {
            spacing(StyleHelper::SpacingTokens::GapHS),
            m_deployConfigurationCombo,
            m_addDeployToolButton,
            m_removeDeployToolButton,
            m_renameDeployButton,
        },
    };

    m_subWidgetsLayout->addWidget(actionsLayout.emerge());
    m_subWidgetsLayout->addWidget(deployWidget);

    deployWidget->setContentsMargins({});
    m_deployLayout = new QVBoxLayout(deployWidget);
    m_deployLayout->setContentsMargins(0, 0, 0, 0);
    m_deployLayout->setSpacing(StyleHelper::SpacingTokens::GapVS);

    m_addDeployMenu = new QMenu(m_addDeployToolButton);
    m_addDeployToolButton->setMenu(m_addDeployMenu);

    initForActiveBuildConfig();

    connect(m_addDeployMenu, &QMenu::aboutToShow,
            this, &DeploySettingsWidget::aboutToShowDeployMenu);
    connect(m_removeDeployToolButton, &QAbstractButton::clicked,
            this, &DeploySettingsWidget::removeDeployConfiguration);
    connect(m_renameDeployButton, &QAbstractButton::clicked,
            this, &DeploySettingsWidget::renameDeployConfiguration);

    connect(m_target, &Target::activeDeployConfigurationChanged,
            &m_guard, [this] { activeDeployConfigurationChanged(); });
    connect(m_target, &Target::activeBuildConfigurationChanged,
            &m_guard, [this] { initForActiveBuildConfig(); });
}

void DeploySettingsWidget::currentDeployConfigurationChanged(int index)
{
    if (m_ignoreChanges.isLocked())
        return;
    BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    QTC_ASSERT(index != -1, bc->setActiveDeployConfiguration(nullptr, SetActive::Cascade); return);
    bc->setActiveDeployConfiguration(
        qobject_cast<DeployConfiguration *>(
            bc->deployConfigurationModel()->projectConfigurationAt(index)),
        SetActive::Cascade);
}

void DeploySettingsWidget::aboutToShowDeployMenu()
{
    m_addDeployMenu->clear();

    for (DeployConfigurationFactory *factory : DeployConfigurationFactory::find(m_target)) {
        QAction *action = m_addDeployMenu->addAction(factory->defaultDisplayName());
        connect(action, &QAction::triggered, this, [factory, this] {
            BuildConfiguration * const bc = m_target->activeBuildConfiguration();
            DeployConfiguration *newDc = factory->create(bc);
            if (!newDc)
                return;
            bc->addDeployConfiguration(newDc);
            bc->setActiveDeployConfiguration(newDc, SetActive::Cascade);
            m_removeDeployToolButton->setEnabled(bc->deployConfigurations().size() > 1);
        });
    }
}

void DeploySettingsWidget::removeDeployConfiguration()
{
    DeployConfiguration *dc = m_target->activeDeployConfiguration();
    if (BuildManager::isBuilding(dc)) {
        QMessageBox box;
        QPushButton *closeAnyway = box.addButton(Tr::tr("Cancel Build && Remove Deploy Configuration"), QMessageBox::AcceptRole);
        QPushButton *cancelClose = box.addButton(Tr::tr("Do Not Remove"), QMessageBox::RejectRole);
        box.setDefaultButton(cancelClose);
        box.setWindowTitle(Tr::tr("Remove Deploy Configuration %1?").arg(dc->displayName()));
        box.setText(Tr::tr("The deploy configuration <b>%1</b> is currently being built.").arg(dc->displayName()));
        box.setInformativeText(Tr::tr("Do you want to cancel the build process and remove the Deploy Configuration anyway?"));
        box.exec();
        if (box.clickedButton() != closeAnyway)
            return;
        BuildManager::cancel();
    } else {
        QMessageBox msgBox(QMessageBox::Question, Tr::tr("Remove Deploy Configuration?"),
                           Tr::tr("Do you really want to delete deploy configuration <b>%1</b>?").arg(dc->displayName()),
                           QMessageBox::Yes|QMessageBox::No, this);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setEscapeButton(QMessageBox::No);
        if (msgBox.exec() == QMessageBox::No)
            return;
    }

    BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    bc->removeDeployConfiguration(dc);

    m_removeDeployToolButton->setEnabled(bc->deployConfigurations().size() > 1);
}

void DeploySettingsWidget::activeDeployConfigurationChanged()
{
    updateDeployConfiguration(m_target->activeDeployConfiguration());
}

void DeploySettingsWidget::renameDeployConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, Tr::tr("Rename..."),
                                         Tr::tr("New name for deploy configuration <b>%1</b>:").
                                            arg(m_target->activeDeployConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeDeployConfiguration()->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueDCName(name);
    if (name.isEmpty())
        return;
    m_target->activeDeployConfiguration()->setDisplayName(name);
}

void DeploySettingsWidget::initForActiveBuildConfig()
{
    disconnect(m_deployConfigurationCombo, &QComboBox::currentIndexChanged,
               this, &DeploySettingsWidget::currentDeployConfigurationChanged);
    BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    ProjectConfigurationModel * const model = QTC_GUARD(bc) ? bc->deployConfigurationModel()
                                                            : nullptr;
    if (model)
        m_deployConfigurationCombo->setModel(model);
    else
        m_deployConfigurationCombo->clear();
    connect(m_deployConfigurationCombo, &QComboBox::currentIndexChanged,
            this, &DeploySettingsWidget::currentDeployConfigurationChanged);

    // Some projects may not support deployment, so we need this:
    // FIXME: Not true anymore? There should always be an active deploy config.
    m_addDeployToolButton->setEnabled(m_target->activeDeployConfiguration());
    m_deployConfigurationCombo->setEnabled(m_target->activeDeployConfiguration());
    m_renameDeployButton->setEnabled(m_target->activeDeployConfiguration());

    m_removeDeployToolButton->setEnabled(bc ? bc->deployConfigurations().count() > 1 : false);
    updateDeployConfiguration(m_target->activeDeployConfiguration());

    updateRemoveToolButtons();
}

void DeploySettingsWidget::updateRemoveToolButtons()
{
    const BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    m_removeDeployToolButton->setEnabled(bc->deployConfigurations().count() > 1);
}

void DeploySettingsWidget::updateDeployConfiguration(DeployConfiguration *dc)
{
    delete m_deployConfigurationWidget;
    m_deployConfigurationWidget = nullptr;
    delete m_deploySteps;
    m_deploySteps = nullptr;

    {
        const GuardLocker locker(m_ignoreChanges);
        m_deployConfigurationCombo->setCurrentIndex(-1);
    }

    m_renameDeployButton->setEnabled(dc);

    if (!dc)
        return;

    const BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    int index = bc->deployConfigurationModel()->indexFor(dc);

    {
        const GuardLocker locker(m_ignoreChanges);
        m_deployConfigurationCombo->setCurrentIndex(index);
    }

    m_deployConfigurationWidget = dc->createConfigWidget();
    if (m_deployConfigurationWidget)
        m_deployLayout->addWidget(m_deployConfigurationWidget);

    m_deploySteps = new BuildStepListWidget(dc->stepList());
    m_deployLayout->addWidget(m_deploySteps);
}

QString DeploySettingsWidget::uniqueDCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList dcNames;
        const BuildConfiguration * const bc = m_target->activeBuildConfiguration();
        QTC_ASSERT(bc, return name);
        const QList<DeployConfiguration *> configurations = bc->deployConfigurations();
        for (DeployConfiguration *dc : configurations) {
            if (dc == bc->activeDeployConfiguration())
                continue;
            dcNames.append(dc->displayName());
        }
        result = makeUniquelyNumbered(result, dcNames);
    }
    return result;
}

void DeploySettingsWidget::updateEnabledState()
{
    const bool enable = m_runConfiguration
                            ? m_runConfiguration->isEnabled(Constants::NORMAL_RUN_MODE)
                            : false;
    const QString reason = m_runConfiguration
                               ? m_runConfiguration->disabledReason(Constants::NORMAL_RUN_MODE)
                               : QString();

    m_disabledText->setVisible(!enable && !reason.isEmpty());
    m_disabledText->setText(reason);
}

// RunSettingsWidget

class RunSettingsWidget : public RunAndDeploySettingsWidgetBase
{
public:
    explicit RunSettingsWidget(Target *target);

private:
    void currentRunConfigurationChanged(int index);
    void showAddRunConfigDialog();
    void cloneRunConfiguration();
    void cloneOtherRunConfiguration();
    void removeRunConfiguration();
    void removeRunConfigurations();
    void removeAllRunConfigurations();
    void activeRunConfigurationChanged();
    void renameRunConfiguration();
    void initForActiveBuildConfig();
    QList<BuildConfiguration *> affectedBuildConfigurations();

    void updateRemoveToolButtons();

    QString uniqueRCName(const QString &name);
    void setConfigurationWidget(RunConfiguration *rc, bool force);

    void addRunControlWidgets();

    void updateEnabledState();

    QPointer<QWidget> m_runConfigurationWidget;
    QPointer<RunConfiguration> m_runConfiguration;
    QVBoxLayout *m_runLayout = nullptr;
    Guard m_ignoreChanges;

    QComboBox *m_runConfigurationCombo;
    QPushButton *m_addRunToolButton;
    QPushButton *m_removeRunToolButton;
    QPushButton *m_removeRunConfigsButton;
    QPushButton *m_removeAllRunConfigsButton;
    QPushButton *m_renameRunButton;
    QPushButton *m_cloneRunButton;
    QPushButton *m_cloneIntoThisButton;
    InfoLabel *m_disabledText;
    QObject m_guard;
};

RunSettingsWidget::RunSettingsWidget(Target *target)
    : RunAndDeploySettingsWidgetBase(target)
{
    setWindowTitle(Tr::tr("Run Settings"));

    m_runConfigurationCombo = new QComboBox(this);
    m_runConfigurationCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_runConfigurationCombo->setMinimumContentsLength(15);
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    m_runConfigurationCombo->view()->setKeyboardSearchFlags(Qt::MatchContains | Qt::MatchWrap);
#endif
    setWheelScrollingWithoutFocusBlocked(m_runConfigurationCombo);

    m_addRunToolButton = new QPushButton(Tr::tr("Add..."), this);
    m_removeRunToolButton = new QPushButton(Tr::tr("Remove"), this);
    m_removeRunConfigsButton = new QPushButton(Tr::tr("Bulk Remove..."), this);
    m_removeAllRunConfigsButton = new QPushButton(Tr::tr("Remove All"), this);
    m_renameRunButton = new QPushButton(Tr::tr("Rename..."), this);
    m_cloneRunButton = new QPushButton(Tr::tr("Clone..."), this);
    m_cloneIntoThisButton = new QPushButton(Tr::tr("Clone into This..."), this);

    auto runWidget = new QWidget(this);

    auto runLabel = new QLabel(Tr::tr("Active run configuration:"), this);
    runLabel->setBuddy(m_runConfigurationCombo);

    using namespace Layouting;
    Form actionsLayout {
        noMargin,
        runLabel,
        Flow {
            spacing(StyleHelper::SpacingTokens::GapHS),
            m_runConfigurationCombo,
            m_addRunToolButton,
            m_removeRunToolButton,
            m_removeRunConfigsButton,
            m_removeAllRunConfigsButton,
            m_renameRunButton,
            m_cloneRunButton,
            m_cloneIntoThisButton,
        },
    };

    m_subWidgetsLayout->addWidget(actionsLayout.emerge());
    m_subWidgetsLayout->addWidget(runWidget);

    runWidget->setContentsMargins({});
    m_runLayout = new QVBoxLayout(runWidget);
    m_runLayout->setContentsMargins(0, 0, 0, 0);
    m_runLayout->setSpacing(5);
    m_disabledText = new InfoLabel({}, InfoLabel::Warning);
    m_runLayout->addWidget(m_disabledText);

    initForActiveBuildConfig();

    connect(m_target, &Target::activeBuildConfigurationChanged,
            &m_guard, [this] { initForActiveBuildConfig(); });

    connect(m_addRunToolButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::showAddRunConfigDialog);
    connect(m_removeRunToolButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::removeRunConfiguration);
    connect(m_removeRunConfigsButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::removeRunConfigurations);
    connect(m_removeAllRunConfigsButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::removeAllRunConfigurations);
    connect(m_renameRunButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::renameRunConfiguration);
    connect(m_cloneRunButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::cloneRunConfiguration);
    connect(m_cloneIntoThisButton, &QAbstractButton::clicked,
            this, &RunSettingsWidget::cloneOtherRunConfiguration);

    connect(m_target, &Target::addedRunConfiguration,
            &m_guard, [this] { updateRemoveToolButtons(); });
    connect(m_target, &Target::removedRunConfiguration,
            &m_guard, [this] { updateRemoveToolButtons(); });
    connect(m_target, &Target::addedDeployConfiguration,
            &m_guard, [this] { updateRemoveToolButtons(); });
    connect(target, &Target::removedDeployConfiguration,
            &m_guard, [this] { updateRemoveToolButtons(); });

    connect(m_target, &Target::activeRunConfigurationChanged,
            &m_guard, [this] { activeRunConfigurationChanged(); });
}

void RunSettingsWidget::showAddRunConfigDialog()
{
    AddRunConfigDialog dlg(m_target->activeBuildConfiguration(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    RunConfigurationCreationInfo rci = dlg.creationInfo();
    QTC_ASSERT(rci.factory, return);
    RunConfiguration *newRC = rci.create(m_target->activeBuildConfiguration());
    if (!newRC)
        return;
    QTC_CHECK(newRC->id() == rci.factory->runConfigurationId());
    newRC->setUniqueId(QUuid::createUuid().toString());
    m_target->activeBuildConfiguration()->addRunConfiguration(newRC, NameHandling::Uniquify);
    for (BuildConfiguration * const bc : newRC->syncableBuildConfigurations()) {
        if (RunConfiguration * const otherNewRc = newRC->clone(bc))
            bc->addRunConfiguration(otherNewRc, NameHandling::Keep);
    }
    newRC->makeActive();
    updateRemoveToolButtons();
}

void RunSettingsWidget::cloneRunConfiguration()
{
    RunConfiguration* activeRunConfiguration = m_target->activeRunConfiguration();

    //: Title of a the cloned RunConfiguration window, text of the window
    QString name = uniqueRCName(
                        QInputDialog::getText(this,
                                              Tr::tr("Clone Configuration"),
                                              Tr::tr("New configuration name:"),
                                              QLineEdit::Normal,
                                              activeRunConfiguration->displayName()));
    if (name.isEmpty())
        return;

    RunConfiguration *newRc = activeRunConfiguration->clone(m_target->activeBuildConfiguration());
    if (!newRc)
        return;

    newRc->setDisplayName(name);
    newRc->setUniqueId(QUuid::createUuid().toString());
    m_target->activeBuildConfiguration()->addRunConfiguration(newRc, NameHandling::Uniquify);
    for (BuildConfiguration * const bc : newRc->syncableBuildConfigurations()) {
        if (RunConfiguration * const otherNewRc = newRc->clone(bc))
            bc->addRunConfiguration(otherNewRc, NameHandling::Keep);
    }
    newRc->makeActive();
}

void RunSettingsWidget::cloneOtherRunConfiguration()
{
    // Paranoia: Guard against project changes during the dialog run.
    const QPointer<RunConfiguration> thisRc = m_target->activeRunConfiguration();
    const QPointer<BuildConfiguration> bc(thisRc->buildConfiguration());

    CloneIntoRunConfigDialog dlg(m_target->activeRunConfiguration());
    if (dlg.exec() == QDialog::Accepted) {
        const RunConfiguration * const otherRc = dlg.source();
        if (bc && otherRc && thisRc && thisRc == m_target->activeRunConfiguration()) {
            thisRc->cloneFromOther(otherRc);
            setConfigurationWidget(thisRc, true); // Force visual update.
        }
    }
}

void RunSettingsWidget::removeRunConfiguration()
{
    RunConfiguration *rc = m_target->activeRunConfiguration();
    QMessageBox msgBox(QMessageBox::Question, Tr::tr("Remove Run Configuration?"),
                       Tr::tr("Do you really want to delete the run configuration <b>%1</b>?").arg(rc->displayName()),
                       QMessageBox::Yes|QMessageBox::No, this);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setEscapeButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::No)
        return;

    rc->forEachLinkedRunConfig([](RunConfiguration *otherRc) {
        otherRc->buildConfiguration()->removeRunConfiguration(otherRc);
    });
    m_target->activeBuildConfiguration()->removeRunConfiguration(rc);
    updateRemoveToolButtons();
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneIntoThisButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::removeRunConfigurations()
{
    RemoveRunConfigsDialog dlg(m_target->activeBuildConfiguration(), this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const QList<BuildConfiguration *> affectedBcs = affectedBuildConfigurations();
    for (BuildConfiguration * const bc : affectedBcs)
        bc->removeRunConfigurations(dlg.runConfigsToRemove());

    updateRemoveToolButtons();
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneIntoThisButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::removeAllRunConfigurations()
{
    QMessageBox msgBox(QMessageBox::Question,
                       Tr::tr("Remove Run Configurations?"),
                       Tr::tr("Do you really want to delete all run configurations?"),
                       QMessageBox::Cancel,
                       this);
    msgBox.addButton(Tr::tr("Delete"), QMessageBox::YesRole);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setEscapeButton(QMessageBox::Cancel);
    if (msgBox.exec() == QMessageBox::Cancel)
        return;

    const QList<BuildConfiguration *> affectedBcs = affectedBuildConfigurations();
    for (BuildConfiguration * const bc : affectedBcs)
        bc->removeAllRunConfigurations();

    updateRemoveToolButtons();
    m_renameRunButton->setEnabled(false);
    m_cloneRunButton->setEnabled(false);
    m_cloneIntoThisButton->setEnabled(false);
}

void RunSettingsWidget::activeRunConfigurationChanged()
{
    if (m_ignoreChanges.isLocked())
        return;

    ProjectConfigurationModel *model = m_target->activeBuildConfiguration()->runConfigurationModel();
    int index = model->indexFor(m_target->activeRunConfiguration());
    {
        const GuardLocker locker(m_ignoreChanges);
        m_runConfigurationCombo->setCurrentIndex(index);
        setConfigurationWidget(
            qobject_cast<RunConfiguration *>(model->projectConfigurationAt(index)), false);
    }
    m_renameRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneRunButton->setEnabled(m_target->activeRunConfiguration());
    m_cloneIntoThisButton->setEnabled(m_target->activeRunConfiguration());
}

void RunSettingsWidget::renameRunConfiguration()
{
    bool ok;
    QString name = QInputDialog::getText(this, Tr::tr("Rename..."),
                                         Tr::tr("New name for run configuration <b>%1</b>:").
                                            arg(m_target->activeRunConfiguration()->displayName()),
                                         QLineEdit::Normal,
                                         m_target->activeRunConfiguration()->displayName(), &ok);
    if (!ok)
        return;

    name = uniqueRCName(name);
    if (name.isEmpty())
        return;

    m_target->activeRunConfiguration()->setDisplayName(name);
    m_target->activeRunConfiguration()->forEachLinkedRunConfig(
        [&name](RunConfiguration *rc) { rc->setDisplayName(name); });
}

void RunSettingsWidget::currentRunConfigurationChanged(int index)
{
    if (m_ignoreChanges.isLocked())
        return;

    RunConfiguration *selectedRunConfiguration = nullptr;
    if (index >= 0)
        selectedRunConfiguration = qobject_cast<RunConfiguration *>
                (m_target->activeBuildConfiguration()->runConfigurationModel()->projectConfigurationAt(index));

    if (selectedRunConfiguration == m_runConfiguration)
        return;

    {
        const GuardLocker locker(m_ignoreChanges);
        selectedRunConfiguration->makeActive();
    }

    // Update the run configuration configuration widget
    setConfigurationWidget(selectedRunConfiguration, false);
}

void RunSettingsWidget::initForActiveBuildConfig()
{
    disconnect(m_runConfigurationCombo, &QComboBox::currentIndexChanged,
            this, &RunSettingsWidget::currentRunConfigurationChanged);
    RunConfiguration *rc = m_target->activeRunConfiguration();
    BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    ProjectConfigurationModel * const model = QTC_GUARD(bc) ? bc->runConfigurationModel() : nullptr;
    if (model) {
        m_runConfigurationCombo->setModel(model);
        m_runConfigurationCombo->setCurrentIndex(model->indexFor(rc));
    } else {
        m_runConfigurationCombo->clear();
    }
    connect(m_runConfigurationCombo, &QComboBox::currentIndexChanged,
            this, &RunSettingsWidget::currentRunConfigurationChanged);
    updateRemoveToolButtons();
    m_renameRunButton->setEnabled(rc);
    m_cloneRunButton->setEnabled(rc);
    m_cloneIntoThisButton->setEnabled(rc);

    setConfigurationWidget(rc, false);
}

QList<BuildConfiguration *> RunSettingsWidget::affectedBuildConfigurations()
{
    switch (ProjectExplorerSettings::get(m_target).syncRunConfigurations.value()) {
    case SyncRunConfigs::Off:
        return {m_target->activeBuildConfiguration()};
    case SyncRunConfigs::SameKit:
        return m_target->buildConfigurations();
    case SyncRunConfigs::All:
        return m_target->project()->allBuildConfigurations();
    }

    QTC_CHECK(false);
    return {};
}

void RunSettingsWidget::updateRemoveToolButtons()
{
    const BuildConfiguration * const bc = m_target->activeBuildConfiguration();
    QTC_ASSERT(bc, return);
    const bool hasRunConfigs = !bc->runConfigurations().isEmpty();
    m_removeRunToolButton->setEnabled(hasRunConfigs);
    m_removeAllRunConfigsButton->setEnabled(hasRunConfigs);
    m_removeRunConfigsButton->setEnabled(hasRunConfigs);
}

void RunSettingsWidget::setConfigurationWidget(RunConfiguration *rc, bool force)
{
    if (!force && rc == m_runConfiguration)
        return;

    delete m_runConfigurationWidget;
    m_runConfigurationWidget = nullptr;
    removeSubWidgets();
    if (!rc)
        return;
    m_runConfigurationWidget = rc->createConfigurationWidget();
    m_runConfiguration = rc;
    if (m_runConfigurationWidget) {
        m_runLayout->addWidget(m_runConfigurationWidget);
        updateEnabledState();
        connect(m_runConfiguration, &RunConfiguration::enabledChanged,
                m_runConfigurationWidget, [this] { updateEnabledState(); });
    }
    addRunControlWidgets();
}

QString RunSettingsWidget::uniqueRCName(const QString &name)
{
    QString result = name.trimmed();
    if (!result.isEmpty()) {
        QStringList rcNames;
        const QList<RunConfiguration *> configurations
            = m_target->activeBuildConfiguration()->runConfigurations();
        for (RunConfiguration *rc : configurations) {
            if (rc == m_target->activeRunConfiguration())
                continue;
            rcNames.append(rc->displayName());
        }
        result = makeUniquelyNumbered(result, rcNames);
    }
    return result;
}

void RunSettingsWidget::addRunControlWidgets()
{
    for (BaseAspect *aspect : m_runConfiguration->aspects()) {
        if (QWidget *rcw = aspect->createConfigWidget()) {
            auto label = new QLabel(this);
            label->setText(aspect->displayName());
            connect(aspect, &GlobalOrProjectAspect::changed, label, [label, aspect] {
                label->setText(aspect->displayName());
            });
            addSubWidget(rcw, label);
        }
    }
}

void RunSettingsWidget::updateEnabledState()
{
    const bool enable = m_runConfiguration
                            ? m_runConfiguration->isEnabled(Constants::NORMAL_RUN_MODE)
                            : false;
    const QString reason = m_runConfiguration
                               ? m_runConfiguration->disabledReason(Constants::NORMAL_RUN_MODE)
                               : QString();

    m_runConfigurationWidget->setEnabled(enable);

    m_disabledText->setVisible(!enable && !reason.isEmpty());
    m_disabledText->setText(reason);
}

// "Factories"

QWidget *createDeploySettingsWidget(Target *target)
{
    return new DeploySettingsWidget(target);
}

QWidget *createRunSettingsWidget(Target *target)
{
    return new RunSettingsWidget(target);
}

} // ProjectExplorer::Internal
