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

#include "targetsettingspanel.h"

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "buildsettingspropertiespage.h"
#include "ipotentialkit.h"
#include "kit.h"
#include "kitmanager.h"
#include "panelswidget.h"
#include "project.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"
#include "projectwindow.h"
#include "runsettingspropertiespage.h"
#include "session.h"
#include "target.h"
#include "targetsetuppage.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>

#include <cmath>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

class TargetSetupPageWrapper : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(TargetSettingsPanelWidget)

public:
    explicit TargetSetupPageWrapper(Project *project);

protected:
    void keyReleaseEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
            event->accept();
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            event->accept();
            done();
        }
    }

private:
    void done()
    {
        m_targetSetupPage->setupProject(m_project);
        Core::ModeManager::activateMode(Core::Constants::MODE_EDIT);
    }

    void cancel()
    {
        ProjectExplorerPlugin::unloadProject(m_project);
    }

    void kitUpdated(ProjectExplorer::Kit *k)
    {
        if (k == KitManager::defaultKit())
            updateNoteText();
    }

    void completeChanged()
    {
        m_configureButton->setEnabled(m_targetSetupPage->isComplete());
    }

    void updateNoteText();

    Project *m_project;
    TargetSetupPage *m_targetSetupPage;
    QPushButton *m_configureButton;
};

TargetSetupPageWrapper::TargetSetupPageWrapper(Project *project)
    : m_project(project)
{
    m_targetSetupPage = new TargetSetupPage(this);
    m_targetSetupPage->setUseScrollArea(false);
    m_targetSetupPage->setProjectPath(project->projectFilePath().toString());
    m_targetSetupPage->setRequiredKitPredicate(project->requiredKitPredicate());
    m_targetSetupPage->setPreferredKitPredicate(project->preferredKitPredicate());
    m_targetSetupPage->setProjectImporter(project->projectImporter());
    m_targetSetupPage->initializePage();
    m_targetSetupPage->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    updateNoteText();

    auto box = new QDialogButtonBox(this);

    m_configureButton = new QPushButton(this);
    m_configureButton->setText(tr("Configure Project"));
    box->addButton(m_configureButton, QDialogButtonBox::AcceptRole);

    auto hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(box);

    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_targetSetupPage);
    layout->addLayout(hbox);
    layout->addStretch(10);

    completeChanged();

    connect(m_configureButton, &QAbstractButton::clicked,
            this, &TargetSetupPageWrapper::done);
    connect(m_targetSetupPage, &QWizardPage::completeChanged,
            this, &TargetSetupPageWrapper::completeChanged);
    connect(KitManager::instance(), &KitManager::defaultkitChanged,
            this, &TargetSetupPageWrapper::updateNoteText);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &TargetSetupPageWrapper::kitUpdated);
}

void TargetSetupPageWrapper::updateNoteText()
{
    Kit *k = KitManager::defaultKit();

    QString text;
    bool showHint = false;
    if (!k) {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator cannot parse the project, because no kit "
                  "has been set up.")
                .arg(m_project->displayName());
        showHint = true;
    } else if (k->isValid()) {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator uses the kit <b>%2</b> to parse the project.")
                .arg(m_project->displayName())
                .arg(k->displayName());
        showHint = false;
    } else {
        text = tr("The project <b>%1</b> is not yet configured.<br/>"
                  "Qt Creator uses the <b>invalid</b> kit <b>%2</b> to parse the project.")
                .arg(m_project->displayName())
                .arg(k->displayName());
        showHint = true;
    }

    m_targetSetupPage->setNoteText(text);
    m_targetSetupPage->showOptionsHint(showHint);
}

//
// TargetSettingsPanelItem
//

class TargetGroupItemPrivate : public QObject
{
    Q_DECLARE_TR_FUNCTIONS(TargetSettingsPanelItem)

public:
    TargetGroupItemPrivate(TargetGroupItem *q, Project *project);
    ~TargetGroupItemPrivate();

    void handleRemovedKit(Kit *kit);
    void handleAddedKit(Kit *kit);
    void handleUpdatedKit(Kit *kit);

    void handleTargetAdded(Target *target);
    void handleTargetRemoved(Target *target);
    void handleTargetChanged(Target *target);

    void ensureWidget();
    void rebuildContents();

    TargetGroupItem *q;
    QString m_displayName;
    Project *m_project;

    QPointer<QWidget> m_noKitLabel;
    QPointer<QWidget> m_configurePage;
    QPointer<QWidget> m_configuredPage;
};

void TargetGroupItemPrivate::ensureWidget()
{
    if (!m_noKitLabel) {
        m_noKitLabel = new QWidget;
        m_noKitLabel->setFocusPolicy(Qt::NoFocus);

        auto label = new QLabel;
        label->setText(tr("No kit defined in this project."));
        QFont f = label->font();
        f.setPointSizeF(f.pointSizeF() * 1.4);
        f.setBold(true);
        label->setFont(f);
        label->setMargin(10);
        label->setAlignment(Qt::AlignTop);

        auto layout = new QVBoxLayout(m_noKitLabel);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(label);
        layout->addStretch(10);
    }

    if (!m_configurePage) {
        auto widget = new TargetSetupPageWrapper(m_project);
        m_configurePage = new PanelsWidget(tr("Configure Project"),
                                           QIcon(":/projectexplorer/images/unconfigured.png"),
                                           widget);
        m_configurePage->setFocusProxy(widget);
    }

    if (!m_configuredPage) {
        auto widget = new QWidget;
        auto label = new QLabel("This project is already configured.");
        auto layout = new QVBoxLayout(widget);
        layout->setMargin(0);
        layout->addWidget(label);
        layout->addStretch(10);
        m_configuredPage = new PanelsWidget(tr("Configure Project"),
                                            QIcon(":/projectexplorer/images/unconfigured.png"),
                                            widget);
    }
}

//
// Third level: The per-kit entries (inactive or with a 'Build' and a 'Run' subitem)
//
class TargetItem : public TypedTreeItem<TreeItem, TargetGroupItem>
{
    Q_DECLARE_TR_FUNCTIONS(TargetSettingsPanelWidget)

public:
    enum { DefaultPage = 0 }; // Build page.

    TargetItem(Project *project, Id kitId)
        : m_project(project), m_kitId(kitId)
    {
        updateSubItems();
    }

    Target *target() const
    {
        return m_project->target(m_kitId);
    }

    void updateSubItems();

    Qt::ItemFlags flags(int column) const override
    {
        Q_UNUSED(column)
        return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole: {
            if (Kit *kit = KitManager::kit(m_kitId))
                return kit->displayName();
            break;
        }

        case Qt::DecorationRole: {
            const Kit *k = KitManager::kit(m_kitId);
            QTC_ASSERT(k, return QVariant());
            if (!isEnabled())
                return kitIconWithOverlay(*k, IconOverlay::Add);
            if (!k->isValid())
                return kitIconWithOverlay(*k, IconOverlay::Error);
            if (k->hasWarning())
                return kitIconWithOverlay(*k, IconOverlay::Warning);
            return k->icon();
        }

        case Qt::TextColorRole: {
            if (!isEnabled())
                return Utils::creatorTheme()->color(Theme::TextColorDisabled);
            break;
        }

        case Qt::FontRole: {
            QFont font = parent()->data(column, role).value<QFont>();
            if (TargetItem *targetItem = parent()->currentTargetItem())
                if (targetItem->target()->id() == m_kitId
                        && m_project == SessionManager::startupProject())
                    font.setBold(true);
            return font;
        }

        case Qt::ToolTipRole: {
            Kit *k = KitManager::kit(m_kitId);
            QTC_ASSERT(k, return QVariant());
            QString toolTip;
            if (!isEnabled())
                toolTip = "<h3>" + tr("Click to activate:") + "</h3>";
            toolTip += k->toHtml();
            return toolTip;
        }

        case PanelWidgetRole:
        case ActiveItemRole: {
            if (m_currentChild >= 0 && m_currentChild < childCount())
                return childAt(m_currentChild)->data(column, role);
            break;
        }

        default:
            break;
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)

        if (role == ContextMenuItemAdderRole) {
            QMenu *menu = data.value<QMenu *>();
            addToContextMenu(menu);
            return true;
        }

        if (role == ItemActivatedDirectlyRole) {
            QTC_ASSERT(!data.isValid(), return false);
            if (!isEnabled()) {
                m_currentChild = DefaultPage;
                Kit *k = KitManager::kit(m_kitId);
                m_project->addTarget(m_project->createTarget(k));
            } else {
                // Go to Run page, when on Run previously etc.
                TargetItem *previousItem = parent()->currentTargetItem();
                m_currentChild = previousItem ? previousItem->m_currentChild : DefaultPage;
                SessionManager::setActiveTarget(m_project, target(), SetActive::Cascade);
                parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)),
                                  ItemActivatedFromBelowRole);
            }
            return true;
        }

        if (role == ItemActivatedFromBelowRole) {
            // I.e. 'Build' and 'Run' items were present and user clicked on them.
            int child = indexOf(data.value<TreeItem *>());
            QTC_ASSERT(child != -1, return false);
            m_currentChild = child; // Triggered from sub-item.
            SessionManager::setActiveTarget(m_project, target(), SetActive::Cascade);
            // Propagate Build/Run selection up.
            parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)),
                              ItemActivatedFromBelowRole);
            return true;
        }

        if (role == ItemActivatedFromAboveRole) {
            // Usually programmatic activation, e.g. after opening the Project mode.
            SessionManager::setActiveTarget(m_project, target(), SetActive::Cascade);
            return true;
        }
        return false;
    }

    void addToContextMenu(QMenu *menu)
    {
        Kit *kit = KitManager::kit(m_kitId);
        QTC_ASSERT(kit, return);
        const QString kitName = kit->displayName();
        const QString projectName = m_project->displayName();

        QAction *enableAction = menu->addAction(tr("Enable Kit \"%1\" for Project \"%2\"").arg(kitName, projectName));
        enableAction->setEnabled(m_kitId.isValid() && !isEnabled());
        QObject::connect(enableAction, &QAction::triggered, [this, kit] {
            m_project->addTarget(m_project->createTarget(kit));
        });

        QAction *disableAction = menu->addAction(tr("Disable Kit \"%1\" for Project \"%2\"").arg(kitName, projectName));
        disableAction->setEnabled(m_kitId.isValid() && isEnabled());
        QObject::connect(disableAction, &QAction::triggered, [this, kit] {
            Target *t = target();
            QTC_ASSERT(t, return);
            QString kitName = t->displayName();
            if (BuildManager::isBuilding(t)) {
                QMessageBox box;
                QPushButton *closeAnyway = box.addButton(tr("Cancel Build and Disable Kit in This Project"), QMessageBox::AcceptRole);
                QPushButton *cancelClose = box.addButton(tr("Do Not Remove"), QMessageBox::RejectRole);
                box.setDefaultButton(cancelClose);
                box.setWindowTitle(tr("Disable Kit %1 in This Project?").arg(kitName));
                box.setText(tr("The kit <b>%1</b> is currently being built.").arg(kitName));
                box.setInformativeText(tr("Do you want to cancel the build process and remove the kit anyway?"));
                box.exec();
                if (box.clickedButton() != closeAnyway)
                    return;
                BuildManager::cancel();
            }

            QCoreApplication::processEvents();

            m_project->removeTarget(t);
        });

        QMenu *copyMenu = menu->addMenu(tr("Copy Steps From Another Kit..."));
        if (m_kitId.isValid() && m_project->target(m_kitId)) {
            const QList<Kit *> kits = KitManager::kits();
            for (Kit *kit : kits) {
                QAction *copyAction = copyMenu->addAction(kit->displayName());
                if (kit->id() == m_kitId || !m_project->target(kit->id())) {
                    copyAction->setEnabled(false);
                } else {
                    QObject::connect(copyAction, &QAction::triggered, [this, kit] {
                        Target *thisTarget = m_project->target(m_kitId);
                        Target *sourceTarget = m_project->target(kit->id());
                        Project::copySteps(sourceTarget, thisTarget);
                    });
                }
            }
        } else {
            copyMenu->setEnabled(false);
        }
    }

    bool isEnabled() const { return target() != 0; }

public:
    QPointer<Project> m_project; // Not owned.
    Id m_kitId;
    int m_currentChild = DefaultPage; // Use run page by default.

private:
    enum class IconOverlay {
        Add,
        Warning,
        Error
    };

    static QIcon kitIconWithOverlay(const Kit &kit, IconOverlay overlayType)
    {
        QIcon overlayIcon;
        switch (overlayType) {
        case IconOverlay::Add: {
            static const QIcon add = Utils::Icons::OVERLAY_ADD.icon();
            overlayIcon = add;
            break;
        }
        case IconOverlay::Warning: {
            static const QIcon warning = Utils::Icons::OVERLAY_WARNING.icon();
            overlayIcon = warning;
            break;
        }
        case IconOverlay::Error: {
            static const QIcon err = Utils::Icons::OVERLAY_ERROR.icon();
            overlayIcon = err;
            break;
        }
        }
        const QSize iconSize(16, 16);
        const QRect iconRect(QPoint(), iconSize);
        QPixmap result(iconSize * qApp->devicePixelRatio());
        result.fill(Qt::transparent);
        result.setDevicePixelRatio(qApp->devicePixelRatio());
        QPainter p(&result);
        kit.icon().paint(&p, iconRect, Qt::AlignCenter,
                         overlayType == IconOverlay::Add ? QIcon::Disabled : QIcon::Normal);
        overlayIcon.paint(&p, iconRect);
        return result;
    }
};

//
// Fourth level: The 'Build' and 'Run' sub-items.
//

class BuildOrRunItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(TargetSettingsPanelWidget)

public:
    enum SubIndex { BuildPage = 0, RunPage = 1 };

    BuildOrRunItem(Project *project, Id kitId, SubIndex subIndex)
        : m_project(project), m_kitId(kitId), m_subIndex(subIndex)
    {
    }

    ~BuildOrRunItem()
    {
        delete m_panel;
    }

    Target *target() const
    {
        return m_project->target(m_kitId);
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole: {
            switch (m_subIndex) {
            case BuildPage:
                return tr("Build");
            case RunPage:
                return tr("Run");
            }
            break;
        }

        case Qt::ToolTipRole:
            return parent()->data(column, role);

        case PanelWidgetRole:
            return QVariant::fromValue(panel());

        case ActiveItemRole:
            return QVariant::fromValue<TreeItem *>(const_cast<BuildOrRunItem *>(this));

        case KitIdRole:
            return m_kitId.toSetting();

        case Qt::DecorationRole: {
            switch (m_subIndex) {
            case BuildPage: {
                static const QIcon buildIcon = ProjectExplorer::Icons::BUILD_SMALL.icon();
                return buildIcon;
            }
            case RunPage: {
                static const QIcon runIcon = Utils::Icons::RUN_SMALL.icon();
                return runIcon;
            }
            }
            break;
        }

        default:
            break;
        }

        return QVariant();
    }

    Qt::ItemFlags flags(int column) const override
    {
        return parent()->flags(column);
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (role == ItemActivatedDirectlyRole) {
            parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)),
                              ItemActivatedFromBelowRole);
            return true;
        }

        return parent()->setData(column, data, role);
    }

    QWidget *panel() const
    {
        if (!m_panel) {
            m_panel = (m_subIndex == RunPage)
                    ? new PanelsWidget(RunSettingsWidget::tr("Run Settings"),
                                       QIcon(":/projectexplorer/images/RunSettings.png"),
                                       new RunSettingsWidget(target()))
                    : new PanelsWidget(QCoreApplication::translate("BuildSettingsPanel", "Build Settings"),
                                       QIcon(":/projectexplorer/images/BuildSettings.png"),
                                       new BuildSettingsWidget(target()));
        }
        return m_panel;
    }

public:
    Project *m_project; // Not owned.
    Id m_kitId;
    mutable QPointer<QWidget> m_panel; // Owned.
    const SubIndex m_subIndex;
};

//
// Also third level:
//
class PotentialKitItem : public TypedTreeItem<TreeItem, TargetGroupItem>
{
    Q_DECLARE_TR_FUNCTIONS(TargetSettingsPanelWidget)

public:
    PotentialKitItem(Project *project, IPotentialKit *potentialKit)
        : m_project(project), m_potentialKit(potentialKit)
    {}

    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return m_potentialKit->displayName();

        if (role == Qt::FontRole) {
            QFont font = parent()->data(column, role).value<QFont>();
            font.setItalic(true);
            return font;
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        Q_UNUSED(column)
        if (role == ContextMenuItemAdderRole) {
            QMenu *menu = data.value<QMenu *>();
            auto enableAction = menu->addAction(tr("Enable Kit"));
            enableAction->setEnabled(!isEnabled());
            QObject::connect(enableAction, &QAction::triggered, [this] {
                m_potentialKit->executeFromMenu();
            });
            return true;
        }

        return false;
    }

    Qt::ItemFlags flags(int column) const override
    {
        Q_UNUSED(column)
        if (isEnabled())
            return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return Qt::ItemIsSelectable;
    }

    bool isEnabled() const { return m_potentialKit->isEnabled(); }

    Project *m_project;
    IPotentialKit *m_potentialKit;
};

TargetGroupItem::TargetGroupItem(const QString &displayName, Project *project)
    : d(new TargetGroupItemPrivate(this, project))
{
    d->m_displayName = displayName;
    QObject::connect(project, &Project::addedTarget,
            d, &TargetGroupItemPrivate::handleTargetAdded,
            Qt::QueuedConnection);
    QObject::connect(project, &Project::removedTarget,
            d, &TargetGroupItemPrivate::handleTargetRemoved);
    QObject::connect(project, &Project::activeTargetChanged,
            d, &TargetGroupItemPrivate::handleTargetChanged, Qt::QueuedConnection);
}

TargetGroupItem::~TargetGroupItem()
{
    delete d;
}

TargetGroupItemPrivate::TargetGroupItemPrivate(TargetGroupItem *q, Project *project)
    : q(q), m_project(project)
{
    // force a signal since the index has changed
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &TargetGroupItemPrivate::handleAddedKit);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &TargetGroupItemPrivate::handleRemovedKit);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &TargetGroupItemPrivate::handleUpdatedKit);

    rebuildContents();
}

TargetGroupItemPrivate::~TargetGroupItemPrivate()
{
    disconnect();

    delete m_noKitLabel;
    delete m_configurePage;
    delete m_configuredPage;
}

QVariant TargetGroupItem::data(int column, int role) const
{
    if (role == Qt::DisplayRole)
        return d->m_displayName;

    if (role == ActiveItemRole) {
        if (TargetItem *item = currentTargetItem())
            return item->data(column, role);
        return QVariant::fromValue<TreeItem *>(const_cast<TargetGroupItem *>(this));
    }

    if (role == PanelWidgetRole) {
        if (TargetItem *item = currentTargetItem())
            return item->data(column, role);

        d->ensureWidget();
        return QVariant::fromValue<QWidget *>(d->m_configurePage.data());
    }

    return QVariant();
}

bool TargetGroupItem::setData(int column, const QVariant &data, int role)
{
    Q_UNUSED(data)
    if (role == ItemActivatedFromBelowRole || role == ItemUpdatedFromBelowRole) {
        // Bubble up to trigger setting the active project.
        parent()->setData(column, QVariant::fromValue(static_cast<TreeItem *>(this)), role);
        return true;
    }

    return false;
}

Qt::ItemFlags TargetGroupItem::flags(int) const
{
    return Qt::NoItemFlags;
}

TargetItem *TargetGroupItem::currentTargetItem() const
{
    return targetItem(d->m_project->activeTarget());
}

TargetItem *TargetGroupItem::targetItem(Target *target) const
{
    if (target) {
        Id needle = target->id(); // Unconfigured project have no active target.
        return findFirstLevelChild([this, needle](TargetItem *item) { return item->m_kitId == needle; });
    }
    return 0;
}

void TargetGroupItemPrivate::handleRemovedKit(Kit *kit)
{
    Q_UNUSED(kit);
    rebuildContents();
}

void TargetGroupItemPrivate::handleUpdatedKit(Kit *kit)
{
    Q_UNUSED(kit);
    rebuildContents();
}

void TargetGroupItemPrivate::handleAddedKit(Kit *kit)
{
    if (m_project->supportsKit(kit))
        q->appendChild(new TargetItem(m_project, kit->id()));
}

void TargetItem::updateSubItems()
{
    if (childCount() == 0 && isEnabled())
        m_currentChild = DefaultPage; // We will add children below.
    removeChildren();
    if (isEnabled()) {
        appendChild(new BuildOrRunItem(m_project, m_kitId, BuildOrRunItem::BuildPage));
        appendChild(new BuildOrRunItem(m_project, m_kitId, BuildOrRunItem::RunPage));
    }
}

void TargetGroupItemPrivate::rebuildContents()
{
    q->removeChildren();

    const QList<Kit *> kits = KitManager::sortKits(KitManager::kits([this](const Kit *kit) {
        return m_project->supportsKit(const_cast<Kit *>(kit));
    }));
    for (Kit *kit : kits)
        q->appendChild(new TargetItem(m_project, kit->id()));

    if (q->parent())
        q->parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(q)),
                             ItemUpdatedFromBelowRole);
}

void TargetGroupItemPrivate::handleTargetAdded(Target *target)
{
    if (TargetItem *item = q->targetItem(target))
        item->updateSubItems();
    q->update();
}

void TargetGroupItemPrivate::handleTargetRemoved(Target *target)
{
    if (TargetItem *item = q->targetItem(target))
        item->updateSubItems();
    q->parent()->setData(0, QVariant::fromValue(static_cast<TreeItem *>(q)),
                         ItemDeactivatedFromBelowRole);
}

void TargetGroupItemPrivate::handleTargetChanged(Target *target)
{
    if (TargetItem *item = q->targetItem(target))
        item->updateSubItems();
    q->setData(0, QVariant(), ItemActivatedFromBelowRole);
}

} // Internal
} // ProjectExplorer
