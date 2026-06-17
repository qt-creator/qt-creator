// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitoptionspage.h"

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevicefactory.h"
#include "filterkitaspectsdialog.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitdata.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "task.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/groupedmodel.h>
#include <utils/guiutils.h>
#include <utils/guard.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QAction>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSet>
#include <QSizePolicy>
#include <QToolButton>

const char WORKING_COPY_KIT_ID[] = "modified kit";

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

// KitModel

class KitModel final : public TypedGroupedModel<KitData>
{
    Q_OBJECT

public:
    explicit KitModel();

    int rowForId(Id kitId) const;
    int rowForOriginalKit(Kit *k) const;
    Kit *kitForRow(int row) const { return KitManager::kit(item(row).m_id); }

    void apply() final;

    int cloneRow(int row) final;
    void markRemoved(int row) final;

    int markForAddition(Kit *baseKit);

    bool isNameUnique(int row) const;

    Kit *modifiedKit() { return &m_modifiedKit; }
    void commitModifiedKit(int row);

signals:
    void kitStateChanged();

private:
    QVariant variantData(int row, int column, int role) const final;
    void addKit(Kit *k);
    void updateKit(Kit *k);
    void removeKit(Kit *k);
    void changeDefaultKit();

    bool m_isRegistering = false;
    Kit m_modifiedKit{Id(WORKING_COPY_KIT_ID)};
};

KitModel::KitModel()
{
    setShowDefault(true);
    setHeader({Tr::tr("Name")});
    setFilters(Constants::msgAutoDetected(), {{Tr::tr("Manual"), [this](int row) {
        return !item(row).m_detectionSource.isAutoDetected();
    }}});

    if (KitManager::isLoaded()) {
        for (Kit *k : KitManager::sortedKits())
            addKit(k);
        changeDefaultKit();
    }
    setDefaultRow(defaultRow());

    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitModel::addKit);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &KitModel::updateKit);
    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &KitModel::updateKit);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitModel::removeKit);
    connect(KitManager::instance(), &KitManager::defaultkitChanged,
            this, &KitModel::changeDefaultKit);
}

static QString displayNameOf(Kit *kit, const KitData &d)
{
    const QString name = d.unexpandedDisplayName();
    if (kit)
        return kit->macroExpander()->expand(name);
    Kit tempKit{Id(WORKING_COPY_KIT_ID)};
    tempKit.copyFrom(d);
    return tempKit.macroExpander()->expand(name);
}

QVariant KitModel::variantData(int row, int /*column*/, int role) const
{
    const KitData d = item(row);
    Kit *kit = KitManager::kit(d.m_id);
    switch (role) {
    case Qt::DisplayRole:
        return displayNameOf(kit, d);
    case Qt::DecorationRole:
        if (kit) {
            if (!kit->isValid())
                return Icons::CRITICAL.icon();
            if (kit->hasWarning() || !isNameUnique(row))
                return Icons::WARNING.icon();
            return kit->icon();
        } else if (!isNameUnique(row)) {
            return Icons::WARNING.icon();
        }
        return d.icon();
    case Qt::ToolTipRole: {
        Tasks tmp;
        if (!isNameUnique(row))
            tmp.append(CompileTask(Task::Warning, Tr::tr("Display name is not unique.")));
        Kit tempKit{Id(WORKING_COPY_KIT_ID)};
        tempKit.copyFrom(d);
        return tempKit.toHtml(tmp);
    }
    default:
        return {};
    }
}

int KitModel::rowForOriginalKit(Kit *k) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).m_id == k->id())
            return row;
    }
    return -1;
}

int KitModel::rowForId(Id kitId) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).m_id == kitId)
            return row;
    }
    return -1;
}

void KitModel::commitModifiedKit(int row)
{
    QTC_ASSERT(row >= 0 && row < itemCount(), return);
    KitData d = m_modifiedKit.kitData();
    d.m_id = item(row).m_id;
    setVolatileItem(row, d);
}

void KitModel::apply()
{
    // Collect kits to deregister (removed rows)
    QList<Kit *> kitsToDeregister;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            if (Kit *kit = KitManager::kit(item(row).m_id))
                kitsToDeregister.append(kit);
        }
    }

    // Apply non-removed dirty/added rows
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        if (!isAdded(row) && !isDirty(row))
            continue;
        const KitData d = item(row);
        if (Kit *kit = KitManager::kit(d.m_id)) {
            kit->copyFrom(d);
            KitManager::notifyAboutUpdate(kit);
        } else {
            m_isRegistering = true;
            KitManager::registerKit([&](Kit *k) { k->copyFrom(d); });
            m_isRegistering = false;
        }
    }

    // Apply default kit selection
    if (const int defRow = defaultRow(); defRow >= 0) {
        if (Kit *kit = KitManager::kit(item(defRow).m_id))
            KitManager::setDefaultKit(kit);
    }

    GroupedModel::apply();

    for (Kit *k : kitsToDeregister)
        KitManager::deregisterKit(k);
}

int KitModel::cloneRow(int row)
{
    Q_UNUSED(row)
    return markForAddition(modifiedKit());
}

void KitModel::markRemoved(int row)
{
    const bool wasRemoved = isRemoved(row);
    GroupedModel::markRemoved(row);
    if (wasRemoved && isOriginalDefault(row))
        setVolatileDefaultRow(row);
    notifyAllRowsChanged();
    emit kitStateChanged();
}

int KitModel::markForAddition(Kit *baseKit)
{
    QStringList allNames;
    for (int row = 0; row < itemCount(); ++row)
        allNames << item(row).unexpandedDisplayName();
    const QString baseName = baseKit
        ? Tr::tr("Clone of %1").arg(baseKit->unexpandedDisplayName())
        : Tr::tr("Unnamed");
    const QString newName = Utils::makeUniquelyNumbered(baseName, allNames);

    Kit tempKit{Id(WORKING_COPY_KIT_ID)};
    if (baseKit)
        tempKit.copyFrom(baseKit);
    else
        tempKit.setup();
    tempKit.setUnexpandedDisplayName(newName);

    KitData kd = tempKit.kitData();
    kd.m_detectionSource = DetectionSource::Manual;
    kd.m_id = {};
    const int newRow = itemCount();
    appendVolatileItem(kd);

    if (defaultRow() < 0 || isRemoved(defaultRow()))
        setVolatileDefaultRow(newRow);

    notifyAllRowsChanged();
    return newRow;
}

void KitModel::addKit(Kit *k)
{
    if (m_isRegistering) {
        for (int row = 0; row < itemCount(); ++row) {
            if (isAdded(row) && !item(row).m_id.isValid()) {
                KitData d = item(row);
                d.m_id = k->id();
                setVolatileItem(row, d);
                notifyRowChanged(row);
                return;
            }
        }
        return;
    }

    appendVariant(toVariant(k->kitData()));

    if (k == KitManager::defaultKit())
        setDefaultRow(rowForId(k->id()));

    notifyAllRowsChanged();
    emit kitStateChanged();
}

void KitModel::updateKit(Kit *k)
{
    const int row = rowForOriginalKit(k);
    if (row < 0)
        return;

    if (!isDirty(row)) {
        // External update with no local edits: refresh committed and volatile variant
        resetItem(row, k->kitData());
    }

    notifyAllRowsChanged();
    emit kitStateChanged();
}

void KitModel::removeKit(Kit *k)
{
    const int row = rowForOriginalKit(k);
    if (row < 0)
        return;
    if (isRemoved(row))
        return;  // already pending deregistration via apply()
    removeItem(row);
    notifyAllRowsChanged();
    emit kitStateChanged();
}

void KitModel::changeDefaultKit()
{
    setVolatileDefaultRow(rowForOriginalKit(KitManager::defaultKit()));
}

bool KitModel::isNameUnique(int row) const
{
    const QString name = displayNameOf(kitForRow(row), item(row));
    for (int r = 0; r < itemCount(); ++r) {
        if (r != row && !isRemoved(r) && displayNameOf(kitForRow(r), item(r)) == name)
            return false;
    }
    return true;
}

// KitOptionsPageWidget

class KitOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    KitOptionsPageWidget();
    ~KitOptionsPageWidget() override;

    void kitSelectionChanged(int newRow);
    void addNewKit();
    void updateState();
    void scrollToSelectedKit();

    void apply() final;
    bool isDirty() const final { return m_model.isDirty(); }

private:
    void onDirty();
    void setFocusToName();
    void load(const KitData &workingCopySrc, int row = -1);

    void updateVisibility();
    void addAspectsToWorkingCopy(Layouting::Layout &parent);
    void setIcon();
    void resetIcon();
    void setDisplayName();
    void setFileSystemFriendlyName();
    void workingCopyWasUpdated(Kit *k);
    void showEvent(QShowEvent *event) final;

    QPushButton m_addButton;
    QPushButton m_filterButton;
    QPushButton m_defaultFilterButton;

    KitModel m_model;
    GroupedView m_groupedView{m_model};

    QWidget m_detailWidget;
    QToolButton m_iconButton;
    QLineEdit m_nameEdit;
    QLineEdit m_fileSystemFriendlyNameLineEdit;
    QList<KitAspect *> m_kitAspects;
    bool m_fixingKit = false;
    bool m_loading = false;
};

KitOptionsPageWidget::KitOptionsPageWidget()
{
    m_addButton.setText(Tr::tr("Add"));
    m_filterButton.setText(Tr::tr("Settings Filter..."));
    m_filterButton.setToolTip(Tr::tr("Choose which settings to display for this kit."));
    m_defaultFilterButton.setText(Tr::tr("Default Settings Filter..."));
    m_defaultFilterButton.setToolTip(Tr::tr("Choose which kit settings to display by default."));

    connect(&m_model, &Internal::KitModel::kitStateChanged,
            this, &KitOptionsPageWidget::updateState);

    // Build the kit detail panel
    m_detailWidget.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto nameLabel = new QLabel(Tr::tr("Name:"));
    nameLabel->setToolTip(Tr::tr("Kit name and icon."));

    const QString fsToolTip =
        "<p>" + Tr::tr("The name of the kit suitable for generating "
               "directory names. This value is used for the variable %1, "
               "which for example determines the name of the shadow build directory.")
               .arg("<i>Kit:FileSystemName</i>") + "</p>";
    m_fileSystemFriendlyNameLineEdit.setToolTip(fsToolTip);
    static const QRegularExpression fileSystemFriendlyNameRegexp(QLatin1String("^[A-Za-z0-9_-]*$"));
    QTC_CHECK(fileSystemFriendlyNameRegexp.isValid());
    m_fileSystemFriendlyNameLineEdit.setValidator(
        new QRegularExpressionValidator(fileSystemFriendlyNameRegexp,
                                        &m_fileSystemFriendlyNameLineEdit));

    auto fsLabel = new QLabel(Tr::tr("File system name:"));
    fsLabel->setToolTip(fsToolTip);
    connect(&m_fileSystemFriendlyNameLineEdit, &QLineEdit::textChanged,
            this, &KitOptionsPageWidget::setFileSystemFriendlyName);

    m_iconButton.setToolTip(Tr::tr("Kit icon."));
    auto setIconAction = new QAction(Tr::tr("Select Icon..."), this);
    m_iconButton.addAction(setIconAction);
    auto resetIconAction = new QAction(Tr::tr("Reset to Device Default Icon"), this);
    m_iconButton.addAction(resetIconAction);

    connect(&m_iconButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::setIcon);
    connect(setIconAction, &QAction::triggered,
            this, &KitOptionsPageWidget::setIcon);
    connect(resetIconAction, &QAction::triggered,
            this, &KitOptionsPageWidget::resetIcon);
    connect(&m_nameEdit, &QLineEdit::textChanged,
            this, &KitOptionsPageWidget::setDisplayName);

    connect(KitManager::instance(), &KitManager::unmanagedKitUpdated,
            this, &KitOptionsPageWidget::workingCopyWasUpdated);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(&m_nameEdit);
    chooser->addMacroExpanderProvider({this, [this] { return m_model.modifiedKit()->macroExpander(); }});

    using namespace Layouting;
    Grid detailPage {
        withFormAlignment,
        columnStretch(1, 2),
        nameLabel, m_nameEdit, m_iconButton, br,
        fsLabel, m_fileSystemFriendlyNameLineEdit, br,
        noMargin
    };

    addAspectsToWorkingCopy(detailPage);
    detailPage.attachTo(&m_detailWidget);

    m_groupedView.makeDefaultButton().setToolTip(
        Tr::tr("Set as the default kit to use when creating a new project."));

    Column {
        Row {
            m_groupedView.view(),
            Column {
                noMargin,
                m_addButton,
                m_groupedView.cloneButton(),
                m_groupedView.removeButton(),
                m_groupedView.makeDefaultButton(),
                m_filterButton,
                m_defaultFilterButton,
                st,
            }
        },
        m_detailWidget
    }.attachTo(this);

    m_detailWidget.setVisible(false);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, [this](int, int newRow) { kitSelectionChanged(newRow); });
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitOptionsPageWidget::updateState);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitOptionsPageWidget::updateState);
    connect(KitManager::instance(), &KitManager::kitUpdated, this, [this](Kit *k) {
        const int row = m_model.rowForOriginalKit(k);
        const int currentRow = m_groupedView.currentRow();
        if (row == currentRow && currentRow >= 0)
            load(m_model.item(currentRow), currentRow);
        updateState();
    });

    m_groupedView.setCanRemoveRow([this](int row) {
        return !m_model.item(row).m_detectionSource.isSdkProvided();
    });
    connect(&m_groupedView, &GroupedView::currentCloned,
            this, &KitOptionsPageWidget::setFocusToName);

    connect(&m_addButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::addNewKit);
    connect(&m_filterButton, &QAbstractButton::clicked, this, [this] {
        QTC_ASSERT(m_groupedView.currentRow() >= 0, return);
        FilterKitAspectsDialog dlg(m_model.modifiedKit(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_model.modifiedKit()->setIrrelevantAspects(dlg.irrelevantAspects());
            updateVisibility();
        }
    });
    connect(&m_defaultFilterButton, &QAbstractButton::clicked, this, [this] {
        FilterKitAspectsDialog dlg(nullptr, this);
        if (dlg.exec() == QDialog::Accepted) {
            KitManager::setIrrelevantAspects(dlg.irrelevantAspects());
            updateVisibility();
        }
    });

    scrollToSelectedKit();
    updateState();
}

KitOptionsPageWidget::~KitOptionsPageWidget()
{
    qDeleteAll(m_kitAspects);
    m_kitAspects.clear();

    // Make sure our working copy did not get registered somehow:
    QTC_CHECK(!contains(KitManager::kits(), equal(&Kit::id, Id(WORKING_COPY_KIT_ID))));
}

void KitOptionsPageWidget::scrollToSelectedKit()
{
    const int row = m_model.rowForId(
        Core::preselectedOptionsPageItem(Constants::KITS_SETTINGS_PAGE_ID));
    m_groupedView.selectRow(row);
    m_groupedView.scrollToRow(row);
}

void KitOptionsPageWidget::apply()
{
    m_model.apply();
    updateState();
}

void KitOptionsPageWidget::kitSelectionChanged(int newRow)
{
    if (newRow >= 0) {
        load(m_model.item(newRow), newRow);
        m_detailWidget.setVisible(true);
        m_groupedView.scrollToRow(newRow);
    } else {
        m_detailWidget.setVisible(false);
    }

    updateState();
}

void KitOptionsPageWidget::addNewKit()
{
    const int row = m_model.markForAddition(nullptr);
    m_groupedView.selectRow(row);

    if (m_groupedView.currentRow() >= 0)
        setFocusToName();
}


void KitOptionsPageWidget::updateState()
{
    const int row = m_groupedView.currentRow();
    const bool hasRow = row >= 0;
    const bool isRemoved = hasRow && m_model.isRemoved(row);
    m_filterButton.setEnabled(hasRow && !isRemoved);
    m_groupedView.updateButtons();
}

void KitOptionsPageWidget::onDirty()
{
    const int row = m_groupedView.currentRow();
    if (row < 0)
        return;
    m_loading = true;
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->apply();
    m_loading = false;
    m_model.commitModifiedKit(row);
    m_model.notifyAllRowsChanged();
}

void KitOptionsPageWidget::setFocusToName()
{
    m_nameEdit.selectAll();
    m_nameEdit.setFocus();
}

void KitOptionsPageWidget::load(const KitData &workingCopySrc, int row)
{
    m_loading = true;

    m_model.modifiedKit()->copyFrom(workingCopySrc);
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->reload();

    m_iconButton.setIcon(m_model.modifiedKit()->icon());
    m_nameEdit.setText(m_model.modifiedKit()->unexpandedDisplayName());
    m_fileSystemFriendlyNameLineEdit.setText(m_model.modifiedKit()->customFileSystemFriendlyName());

    m_loading = false;

    // KitAspect::refresh() may normalize invalid stored values as a side effect of reload().
    // If the row had no prior user changes, update the committed baseline so the
    // normalization doesn't register as a user edit.
    if (row >= 0 && !m_model.isDirty(row)) {
        KitData normalizedData = m_model.modifiedKit()->kitData();
        normalizedData.m_id = workingCopySrc.m_id;
        if (normalizedData != workingCopySrc)
            m_model.resetItem(row, normalizedData);
    }

    updateVisibility();

    if (m_model.modifiedKit()->detectionSource().isAutoDetected()) {
        for (KitAspect *aspect : std::as_const(m_kitAspects))
            aspect->makeStickySubWidgetsReadOnly();
    }
}


void KitOptionsPageWidget::addAspectsToWorkingCopy(Layouting::Layout &parent)
{
    QHash<Id, KitAspect *> aspectsById;
    for (KitAspectFactory *factory : KitManager::kitAspectFactories()) {
        QTC_ASSERT(factory, continue);

        KitAspect *aspect = factory->createKitAspect(m_model.modifiedKit());
        QTC_ASSERT(aspect, continue);
        QTC_ASSERT(!m_kitAspects.contains(aspect), continue);

        m_kitAspects.append(aspect);
        aspectsById.insert(factory->id(), aspect);

        connect(aspect->mutableAction(), &QAction::toggled,
            this, [this] { if (!m_loading) onDirty(); });
        connect(aspect, &BaseAspect::volatileValueChanged,
            this, [this] { if (!m_loading) onDirty(); });
    }

    QSet<KitAspect *> embedded;
    for (KitAspect * const aspect : std::as_const(m_kitAspects)) {
        QList<KitAspect *> embeddables;
        for (const QList<Id> embeddableIds = aspect->factory()->embeddableAspects();
             const Id &embeddableId : embeddableIds) {
            if (KitAspect * const embeddable = aspectsById.value(embeddableId)) {
                embeddables << embeddable;
                embedded << embeddable;
            }
        }
        aspect->setAspectsToEmbed(embeddables);
    }

    for (KitAspect * const aspect : std::as_const(m_kitAspects)) {
        if (!embedded.contains(aspect))
            aspect->addToLayout(parent);
    }
}

void KitOptionsPageWidget::updateVisibility()
{
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->setVisible(m_model.modifiedKit()->isAspectRelevant(aspect->factory()->id()));
}

void KitOptionsPageWidget::setIcon()
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(m_model.modifiedKit());
    QList<IDeviceFactory *> allDeviceFactories = IDeviceFactory::allDeviceFactories();
    if (deviceType.isValid()) {
        const auto less = [deviceType](const IDeviceFactory *f1, const IDeviceFactory *f2) {
            if (f1->deviceType() == deviceType)
                return true;
            if (f2->deviceType() == deviceType)
                return false;
            return f1->displayName() < f2->displayName();
        };
        Utils::sort(allDeviceFactories, less);
    }
    QMenu iconMenu;
    for (const IDeviceFactory * const factory : std::as_const(allDeviceFactories)) {
        if (factory->icon().isNull())
            continue;
        QAction *action = iconMenu.addAction(factory->icon(),
                                             Tr::tr("Default for %1").arg(factory->displayName()),
                                             [this, factory] {
                                                 m_iconButton.setIcon(factory->icon());
                                                 m_model.modifiedKit()->setDeviceTypeForIcon(
                                                     factory->deviceType());
                                                 onDirty();
                                             });
        action->setIconVisibleInMenu(true);
    }
    iconMenu.addSeparator();
    iconMenu.addAction(PathChooser::browseButtonLabel(), [this] {
        const FilePath path = FileUtils::getOpenFilePath(Tr::tr("Select Icon"),
                                                         m_model.modifiedKit()->iconPath(),
                                                         Tr::tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return;
        const QIcon icon(path.toUrlishString());
        if (icon.isNull())
            return;
        m_iconButton.setIcon(icon);
        m_model.modifiedKit()->setIconPath(path);
        onDirty();
    });
    iconMenu.exec(m_iconButton.mapToGlobal(QPoint(0, 0)));
}

void KitOptionsPageWidget::resetIcon()
{
    m_model.modifiedKit()->setIconPath({});
    onDirty();
}

void KitOptionsPageWidget::setDisplayName()
{
    int pos = m_nameEdit.cursorPosition();
    m_model.modifiedKit()->setUnexpandedDisplayName(m_nameEdit.text());
    m_nameEdit.setCursorPosition(pos);
    if (!m_loading)
        onDirty();
}

void KitOptionsPageWidget::setFileSystemFriendlyName()
{
    if (m_fileSystemFriendlyNameLineEdit.text() == m_model.modifiedKit()->customFileSystemFriendlyName())
        return;
    const int pos = m_fileSystemFriendlyNameLineEdit.cursorPosition();
    m_model.modifiedKit()->setCustomFileSystemFriendlyName(m_fileSystemFriendlyNameLineEdit.text());
    m_fileSystemFriendlyNameLineEdit.setCursorPosition(pos);
    if (!m_loading)
        onDirty();
}

void KitOptionsPageWidget::workingCopyWasUpdated(Kit *k)
{
    if (k != m_model.modifiedKit() || m_fixingKit || m_loading)
        return;

    m_fixingKit = true;
    k->fix();
    m_fixingKit = false;

    for (KitAspect *w : std::as_const(m_kitAspects))
        w->refresh();

    if (k->unexpandedDisplayName() != m_nameEdit.text())
        m_nameEdit.setText(k->unexpandedDisplayName());

    m_fileSystemFriendlyNameLineEdit.setText(k->customFileSystemFriendlyName());
    m_iconButton.setIcon(k->icon());
    updateVisibility();
    onDirty();
}

void KitOptionsPageWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (m_detailWidget.isVisible()) {
        for (KitAspect *aspect : std::as_const(m_kitAspects))
            aspect->refresh();
    }
}

// KitsSettingsPage

class KitsSettingsPage : public Core::IOptionsPage
{
public:
    KitsSettingsPage()
    {
        setId(Constants::KITS_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Kits"));
        setCategory(Constants::KITS_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new Internal::KitOptionsPageWidget; });
    }
};

void setupKitsSettingsPage()
{
    static KitsSettingsPage theKitsSettingsPage;
}

} // ProjectExplorer::Internal

#ifdef WITH_TESTS

#include <QTest>

namespace ProjectExplorer::Internal {

static Kit *addTestKit(const QString &name)
{
    return KitManager::registerKit([&name](Kit *k) {
        k->setUnexpandedDisplayName(name);
    });
}

class KitModelTest : public QObject
{
    Q_OBJECT

private slots:
    void testDefaultSuffix();
    void testCancelDoesNotPersistDefault();
    void testRemoveDefaultAutoSwitches();
    void testApplyCommitsDefault();
    void testApplyWithRemovedKit();
};

void KitModelTest::testDefaultSuffix()
{
    const DirtySettingsGuard guard;
    Kit *kit = addTestKit("SuffixKit");
    KitManager::setDefaultKit(kit);

    KitModel model;
    const int row = model.rowForOriginalKit(kit);
    QVERIFY(row >= 0);
    QVERIFY(model.isDefault(row));
    const QString text = model.data(model.index(row, 0), Qt::DisplayRole).toString();
    QVERIFY2(text.contains("(Default)"), qPrintable(text));

    KitManager::deregisterKit(kit);
}

void KitModelTest::testCancelDoesNotPersistDefault()
{
    const DirtySettingsGuard guard;
    Kit *kit1 = addTestKit("CancelKit1");
    Kit *kit2 = addTestKit("CancelKit2");
    KitManager::setDefaultKit(kit1);

    {
        KitModel model;
        const int row2 = model.rowForOriginalKit(kit2);
        QVERIFY(row2 >= 0);
        model.setVolatileDefaultRow(row2);
        // Destroy model without apply() -- simulates Cancel
    }

    QCOMPARE(KitManager::defaultKit(), kit1);

    KitManager::deregisterKit(kit1);
    KitManager::deregisterKit(kit2);
}

void KitModelTest::testRemoveDefaultAutoSwitches()
{
    const DirtySettingsGuard guard;
    Kit *kit1 = addTestKit("RemoveKit1");
    Kit *kit2 = addTestKit("RemoveKit2");
    KitManager::setDefaultKit(kit1);

    KitModel model;
    const int row1 = model.rowForOriginalKit(kit1);
    QVERIFY(row1 >= 0);
    QVERIFY(model.isDefault(row1));

    model.markRemoved(row1);

    QVERIFY(model.isRemoved(row1));
    const int newDefault = model.defaultRow();
    QVERIFY(newDefault >= 0);
    QVERIFY(newDefault != row1);
    QVERIFY(!model.isRemoved(newDefault));

    model.apply();

    QList<Kit *> kits = KitManager::kits();
    QVERIFY(!kits.contains(kit1));
    QVERIFY(kits.contains(kit2));

    KitManager::deregisterKit(kit2);
}

void KitModelTest::testApplyCommitsDefault()
{
    const DirtySettingsGuard guard;
    Kit *kit1 = addTestKit("ApplyKit1");
    Kit *kit2 = addTestKit("ApplyKit2");
    KitManager::setDefaultKit(kit1);

    KitModel model;
    const int row2 = model.rowForOriginalKit(kit2);
    QVERIFY(row2 >= 0);

    model.setVolatileDefaultRow(row2);
    model.apply();

    QCOMPARE(KitManager::defaultKit(), kit2);

    KitManager::deregisterKit(kit1);
    KitManager::deregisterKit(kit2);
}

void KitModelTest::testApplyWithRemovedKit()
{
    // Regression test for QTCREATORBUG-34340: Apply while a kit is removed
    // must not leave rows with null kit pointers visible during the model reset.
    const DirtySettingsGuard guard;
    Kit *kit1 = addTestKit("RemoveApplyKit1");
    Kit *kit2 = addTestKit("RemoveApplyKit2");

    KitModel model;
    const int row1 = model.rowForOriginalKit(kit1);
    QVERIFY(row1 >= 0);
    model.markRemoved(row1);
    QVERIFY(model.isRemoved(row1));

    bool checkedDuringReset = false;
    QObject::connect(model.groupedDisplayModel(), &QAbstractItemModel::modelReset,
                     &model, [&model, &checkedDuringReset] {
        checkedDuringReset = true;
        for (int row = 0; row < model.itemCount(); ++row)
            QVERIFY(model.kitForRow(row));
    });

    const Id kit1Id = kit1->id();
    const int countBeforeApply = model.itemCount();
    model.apply();

    QVERIFY(checkedDuringReset);
    QCOMPARE(model.itemCount(), countBeforeApply - 1);
    QCOMPARE(model.rowForId(kit1Id), -1);
    QVERIFY(model.rowForOriginalKit(kit2) >= 0);
    QVERIFY(!KitManager::kits().contains(kit1));
    QVERIFY(KitManager::kits().contains(kit2));
    KitManager::deregisterKit(kit2);
}

QObject *createKitModelTest()
{
    return new KitModelTest;
}

} // ProjectExplorer::Internal

#endif // WITH_TESTS

#include "kitoptionspage.moc"
