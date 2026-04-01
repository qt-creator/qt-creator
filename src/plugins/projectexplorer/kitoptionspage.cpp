// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitoptionspage.h"

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevicefactory.h"
#include "filterkitaspectsdialog.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "task.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>
#include <utils/groupedmodel.h>
#include <utils/guard.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QAction>
#include <QApplication>
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
#include <QTreeView>
#include <QVBoxLayout>

#include <memory>

const char WORKING_COPY_KIT_ID[] = "modified kit";

using namespace Core;
using namespace Utils;

namespace ProjectExplorer::Internal {

class KitData final
{
public:
    Kit *kit = nullptr; // Not owned.
    Kit *workingCopy = nullptr;  // Not owned.
    bool hasUniqueName = true;

    bool operator==(const KitData &other) const
    {
        return kit == other.kit;
    }
};

} // namespace ProjectExplorer::Internal

Q_DECLARE_METATYPE(ProjectExplorer::Internal::KitData)

namespace ProjectExplorer::Internal {

class KitManagerConfigWidget final : public QWidget
{
    Q_OBJECT

public:
    KitManagerConfigWidget();
    ~KitManagerConfigWidget() final;

    void setFocusToName();
    void load(Kit *originalKit, Kit *workingCopySrc, bool hasUniqueName);
    void save(Kit *dest);
    void updateVisibility();
    QString validityMessage() const;

    Kit *kit() const { return m_kit; }
    Kit *workingCopy() { return &m_modifiedKit; }

    void addAspectsToWorkingCopy(Layouting::Layout &parent);

signals:
    void dirty();
    void isAutoDetectedChanged();

private:
    void setIcon();
    void resetIcon();
    void setDisplayName();
    void setFileSystemFriendlyName();
    void workingCopyWasUpdated(ProjectExplorer::Kit *k);

    void showEvent(QShowEvent *event) final;

    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QLineEdit *m_fileSystemFriendlyNameLineEdit;
    QList<KitAspect *> m_kitAspects;
    Kit *m_kit = nullptr;
    Kit m_modifiedKit;
    bool m_fixingKit = false;
    bool m_loading = false;
    bool m_hasUniqueName = true;
    mutable QString m_cachedDisplayName;
};

KitManagerConfigWidget::KitManagerConfigWidget()
    : m_iconButton(new QToolButton)
    , m_nameEdit(new QLineEdit)
    , m_fileSystemFriendlyNameLineEdit(new QLineEdit)
    , m_modifiedKit(Id(WORKING_COPY_KIT_ID))
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto label = new QLabel(Tr::tr("Name:"));
    label->setToolTip(Tr::tr("Kit name and icon."));

    QString toolTip =
        Tr::tr("<html><head/><body><p>The name of the kit suitable for generating "
               "directory names. This value is used for the variable <i>%1</i>, "
               "which for example determines the name of the shadow build directory."
               "</p></body></html>").arg(QLatin1String("Kit:FileSystemName"));
    m_fileSystemFriendlyNameLineEdit->setToolTip(toolTip);
    static const QRegularExpression fileSystemFriendlyNameRegexp(QLatin1String("^[A-Za-z0-9_-]*$"));
    QTC_CHECK(fileSystemFriendlyNameRegexp.isValid());
    m_fileSystemFriendlyNameLineEdit->setValidator(new QRegularExpressionValidator(fileSystemFriendlyNameRegexp, m_fileSystemFriendlyNameLineEdit));

    auto fsLabel = new QLabel(Tr::tr("File system name:"));
    fsLabel->setToolTip(toolTip);
    connect(m_fileSystemFriendlyNameLineEdit, &QLineEdit::textChanged,
            this, &KitManagerConfigWidget::setFileSystemFriendlyName);

    using namespace Layouting;
    Grid page {
        withFormAlignment,
        columnStretch(1, 2),
        label, m_nameEdit, m_iconButton, br,
        fsLabel, m_fileSystemFriendlyNameLineEdit, br,
        noMargin
    };

    m_iconButton->setToolTip(Tr::tr("Kit icon."));
    auto setIconAction = new QAction(Tr::tr("Select Icon..."), this);
    m_iconButton->addAction(setIconAction);
    auto resetIconAction = new QAction(Tr::tr("Reset to Device Default Icon"), this);
    m_iconButton->addAction(resetIconAction);

    connect(m_iconButton, &QAbstractButton::clicked,
            this, &KitManagerConfigWidget::setIcon);
    connect(setIconAction, &QAction::triggered,
            this, &KitManagerConfigWidget::setIcon);
    connect(resetIconAction, &QAction::triggered,
            this, &KitManagerConfigWidget::resetIcon);
    connect(m_nameEdit, &QLineEdit::textChanged,
            this, &KitManagerConfigWidget::setDisplayName);

    KitManager *km = KitManager::instance();
    connect(km, &KitManager::unmanagedKitUpdated,
            this, &KitManagerConfigWidget::workingCopyWasUpdated);

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_nameEdit);
    chooser->addMacroExpanderProvider({this, [this] { return m_modifiedKit.macroExpander(); }});

    addAspectsToWorkingCopy(page);

    page.attachTo(this);

    updateVisibility();
    setVisible(false);
}

KitManagerConfigWidget::~KitManagerConfigWidget()
{
    qDeleteAll(m_kitAspects);
    m_kitAspects.clear();

    // Make sure our workingCopy did not get registered somehow:
    QTC_CHECK(!contains(KitManager::kits(), equal(&Kit::id, Id(WORKING_COPY_KIT_ID))));
}

void KitManagerConfigWidget::setFocusToName()
{
    m_nameEdit->selectAll();
    m_nameEdit->setFocus();
}

void KitManagerConfigWidget::load(Kit *originalKit, Kit *workingCopySrc, bool hasUniqueName)
{
    m_kit = originalKit;
    m_hasUniqueName = hasUniqueName;

    m_loading = true;

    m_modifiedKit.copyFrom(workingCopySrc);

    // Reset any read-only state from the previous kit
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->reload();

    m_iconButton->setIcon(m_modifiedKit.icon());
    m_nameEdit->setText(m_modifiedKit.unexpandedDisplayName());
    m_cachedDisplayName.clear();
    m_fileSystemFriendlyNameLineEdit->setText(m_modifiedKit.customFileSystemFriendlyName());

    m_loading = false;

    updateVisibility();

    if (originalKit && originalKit->detectionSource().isAutoDetected()) {
        for (KitAspect *aspect : std::as_const(m_kitAspects))
            aspect->makeStickySubWidgetsReadOnly();
    }
}

void KitManagerConfigWidget::save(Kit *dest)
{
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->apply();
    dest->copyFrom(&m_modifiedKit);
}

QString KitManagerConfigWidget::validityMessage() const
{
    Tasks tmp;
    if (!m_hasUniqueName)
        tmp.append(CompileTask(Task::Warning, Tr::tr("Display name is not unique.")));

    return m_modifiedKit.toHtml(tmp);
}

void KitManagerConfigWidget::addAspectsToWorkingCopy(Layouting::Layout &parent)
{
    QHash<Id, KitAspect *> aspectsById;
    for (KitAspectFactory *factory : KitManager::kitAspectFactories()) {
        QTC_ASSERT(factory, continue);

        KitAspect *aspect = factory->createKitAspect(workingCopy());
        QTC_ASSERT(aspect, continue);
        QTC_ASSERT(!m_kitAspects.contains(aspect), continue);

        m_kitAspects.append(aspect);
        aspectsById.insert(factory->id(), aspect);

        connect(aspect->mutableAction(), &QAction::toggled,
            this, [this] { if (!m_loading) emit dirty(); });
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

void KitManagerConfigWidget::updateVisibility()
{
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->setVisible(m_modifiedKit.isAspectRelevant(aspect->factory()->id()));
}

void KitManagerConfigWidget::setIcon()
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(&m_modifiedKit);
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
                                                 m_iconButton->setIcon(factory->icon());
                                                 m_modifiedKit.setDeviceTypeForIcon(
                                                     factory->deviceType());
                                                 emit dirty();
                                             });
        action->setIconVisibleInMenu(true);
    }
    iconMenu.addSeparator();
    iconMenu.addAction(PathChooser::browseButtonLabel(), [this] {
        const FilePath path = FileUtils::getOpenFilePath(Tr::tr("Select Icon"),
                                                         m_modifiedKit.iconPath(),
                                                         Tr::tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return;
        const QIcon icon(path.toUrlishString());
        if (icon.isNull())
            return;
        m_iconButton->setIcon(icon);
        m_modifiedKit.setIconPath(path);
        emit dirty();
    });
    iconMenu.exec(mapToGlobal(m_iconButton->pos()));
}

void KitManagerConfigWidget::resetIcon()
{
    m_modifiedKit.setIconPath({});
    emit dirty();
    markSettingsDirty();
}

void KitManagerConfigWidget::setDisplayName()
{
    int pos = m_nameEdit->cursorPosition();
    m_cachedDisplayName.clear();
    m_modifiedKit.setUnexpandedDisplayName(m_nameEdit->text());
    m_nameEdit->setCursorPosition(pos);
    if (!m_loading)
        markSettingsDirty();
}

void KitManagerConfigWidget::setFileSystemFriendlyName()
{
    if (m_fileSystemFriendlyNameLineEdit->text() == m_modifiedKit.customFileSystemFriendlyName())
        return;
    const int pos = m_fileSystemFriendlyNameLineEdit->cursorPosition();
    m_modifiedKit.setCustomFileSystemFriendlyName(m_fileSystemFriendlyNameLineEdit->text());
    m_fileSystemFriendlyNameLineEdit->setCursorPosition(pos);
    if (!m_loading)
        markSettingsDirty();
}

void KitManagerConfigWidget::workingCopyWasUpdated(Kit *k)
{
    if (k != &m_modifiedKit || m_fixingKit || m_loading)
        return;

    m_fixingKit = true;
    k->fix();
    m_fixingKit = false;

    for (KitAspect *w : std::as_const(m_kitAspects))
        w->refresh();

    m_cachedDisplayName.clear();

    if (k->unexpandedDisplayName() != m_nameEdit->text())
        m_nameEdit->setText(k->unexpandedDisplayName());

    m_fileSystemFriendlyNameLineEdit->setText(k->customFileSystemFriendlyName());
    m_iconButton->setIcon(k->icon());
    updateVisibility();
    emit dirty();
}

void KitManagerConfigWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->refresh();
}

// KitModel

class KitModel final : public TypedGroupedModel<KitData>
{
    Q_OBJECT

public:
    explicit KitModel();

    int rowForKit(Kit *k) const;
    int rowForId(Id kitId) const;
    int rowForOriginalKit(Kit *k) const;
    bool isDefaultKit(Kit *k) const;
    Kit *workingCopyForRow(int row) const;

    void apply() final;

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

    QString newKitName(const QString &sourceName) const;

signals:
    void kitStateChanged();

private:
    QVariant variantData(const QVariant &v, int column, int role) const final;
    void addKit(Kit *k);
    void updateKit(Kit *k);
    void removeKit(Kit *k);
    void changeDefaultKit();
    void validateKitNames();

    std::vector<std::unique_ptr<Kit>> m_workingCopies;
    bool m_isRegistering = false;
};

KitModel::KitModel()
{
    setShowDefault(true);
    setHeader({Tr::tr("Name")});
    setFilters(Constants::msgAutoDetected(), {{Tr::tr("Manual"), [this](int row) {
        const KitData d = item(row);
        return !d.kit || !d.kit->detectionSource().isAutoDetected();
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

Kit *KitModel::workingCopyForRow(int row) const
{
    QTC_ASSERT(row >= 0 && row < int(m_workingCopies.size()), return nullptr);
    return m_workingCopies.at(row).get();
}

QVariant KitModel::variantData(const QVariant &v, int /*column*/, int role) const
{
    const KitData d = fromVariant(v);
    switch (role) {
    case Qt::DisplayRole:
        return d.workingCopy->displayName();
    case Qt::DecorationRole:
        if (d.workingCopy->isValid() && !d.hasUniqueName)
            return Icons::WARNING.icon();
        return d.workingCopy->displayIcon();
    case Qt::ToolTipRole: {
        Tasks tmp;
        if (!d.hasUniqueName)
            tmp.append(CompileTask(Task::Warning, Tr::tr("Display name is not unique.")));
        return d.workingCopy->toHtml(tmp);
    }
    default:
        return {};
    }
}

int KitModel::rowForKit(Kit *k) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).workingCopy == k)
            return row;
    }
    return -1;
}

int KitModel::rowForOriginalKit(Kit *k) const
{
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).kit == k)
            return row;
    }
    return -1;
}

int KitModel::rowForId(Id kitId) const
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.kit && d.kit->id() == kitId)
            return row;
    }
    return -1;
}

bool KitModel::isDefaultKit(Kit *k) const
{
    const int row = rowForKit(k);
    return row >= 0 && isDefault(row);
}

void KitModel::apply()
{
    // Collect kits to deregister (removed rows)
    QList<Kit *> kitsToDeregister;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row) && item(row).kit)
            kitsToDeregister.append(item(row).kit);
    }

    // Apply non-removed dirty/added rows
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        if (!isAdded(row) && !isDirty(row))
            continue;
        KitData d = item(row);
        Kit *wc = m_workingCopies.at(row).get();
        if (d.kit) {
            d.kit->copyFrom(wc);
            KitManager::notifyAboutUpdate(d.kit);
        } else {
            m_isRegistering = true;
            KitManager::registerKit([&](Kit *k) { k->copyFrom(wc); });
            m_isRegistering = false;
        }
    }

    // Apply default kit selection
    if (const int defRow = defaultRow(); defRow >= 0) {
        if (const KitData d = item(defRow); d.kit)
            KitManager::setDefaultKit(d.kit);
    }

    // Working copies that GroupedModel::apply() will keep
    std::vector<std::unique_ptr<Kit>> newWorkingCopies;
    for (int row = 0; row < itemCount(); ++row) {
        if (!isRemoved(row))
            newWorkingCopies.push_back(std::move(m_workingCopies[row]));
    }

    GroupedModel::apply();
    m_workingCopies = std::move(newWorkingCopies);

    for (Kit *k : kitsToDeregister)
        KitManager::deregisterKit(k);
}

void KitModel::markForRemoval(Kit *k)
{
    const int row = rowForKit(k);
    if (row < 0)
        return;

    if (isDefault(row)) {
        for (int r = 0; r < itemCount(); ++r) {
            if (r != row && !isRemoved(r)) {
                setVolatileDefaultRow(r);
                break;
            }
        }
    }

    const bool wasAdded = isAdded(row);
    markRemoved(row);
    if (wasAdded)
        m_workingCopies.erase(m_workingCopies.begin() + row);

    validateKitNames();
    emit kitStateChanged();
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    const QString newName = newKitName(baseKit ? baseKit->unexpandedDisplayName() : QString());

    // Create working copy before appending the item
    auto wc = std::make_unique<Kit>(Id(WORKING_COPY_KIT_ID));
    m_workingCopies.push_back(std::move(wc));
    Kit *k = m_workingCopies.back().get();

    KitData kd;
    kd.workingCopy = k;
    const int newRow = itemCount();
    appendVolatileItem(kd);

    {
        KitGuard g(k);
        if (baseKit) {
            k->copyFrom(baseKit);
            k->setDetectionSource(DetectionSource::Manual);
        } else {
            k->setup();
        }
        k->setUnexpandedDisplayName(newName);
    }

    bool hasDefault = false;
    for (int r = 0; r < newRow; ++r) {
        if (isDefault(r) && !isRemoved(r)) {
            hasDefault = true;
            break;
        }
    }
    if (!hasDefault)
        setVolatileDefaultRow(newRow);

    return k;
}

QString KitModel::newKitName(const QString &sourceName) const
{
    QList<Kit *> allKits;
    for (int row = 0; row < itemCount(); ++row)
        if (Kit *k = workingCopyForRow(row))
            allKits << k;
    return Kit::newKitName(sourceName, allKits);
}

void KitModel::addKit(Kit *k)
{
    if (m_isRegistering) {
        for (int row = 0; row < itemCount(); ++row) {
            if (isAdded(row) && !item(row).kit) {
                KitData d = item(row);
                d.kit = k;
                setVolatileItem(row, d);
                return;
            }
        }
        return;
    }

    KitData newData;
    newData.kit = k;
    auto wc = std::make_unique<Kit>(Id(WORKING_COPY_KIT_ID));
    wc->copyFrom(k);
    newData.workingCopy = wc.get();
    m_workingCopies.push_back(std::move(wc));
    appendVariant(toVariant(newData));

    if (k == KitManager::defaultKit())
        setDefaultRow(rowForId(k->id()));

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::updateKit(Kit *k)
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.kit != k)
            continue;
        if (!isChanged(row)) {
            // External update with no local edits: refresh the working copy
            m_workingCopies.at(row)->copyFrom(k);
        }
        notifyRowChanged(row);
        break;
    }

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::removeKit(Kit *k)
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.kit != k)
            continue;
        if (isRemoved(row))
            return;  // already pending deregistration via apply()
        if (isDefault(row)) {
            for (int r = 0; r < itemCount(); ++r) {
                if (r != row && !isRemoved(r)) {
                    setVolatileDefaultRow(r);
                    break;
                }
            }
        }
        m_workingCopies.erase(m_workingCopies.begin() + row);
        removeItem(row);
        validateKitNames();
        emit kitStateChanged();
        return;
    }
}

void KitModel::changeDefaultKit()
{
    Kit *defaultKit = KitManager::defaultKit();
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).kit == defaultKit) {
            setVolatileDefaultRow(row);
            return;
        }
    }
    setVolatileDefaultRow(-1);
}

void KitModel::validateKitNames()
{
    QHash<QString, int> nameHash;
    for (int row = 0; row < itemCount(); ++row) {
        if (!isRemoved(row))
            nameHash[workingCopyForRow(row)->displayName()]++;
    }

    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        KitData d = item(row);
        const bool unique = nameHash.value(workingCopyForRow(row)->displayName()) == 1;
        if (d.hasUniqueName != unique) {
            d.hasUniqueName = unique;
            setVolatileItem(row, d);
            notifyRowChanged(row);
        }
    }
}

// KitOptionsPageWidget

class KitOptionsPageWidget : public Core::IOptionsPageWidget
{
public:
    KitOptionsPageWidget();

    Kit *currentKit() const;

    void kitSelectionChanged(int oldRow, int newRow);
    void addNewKit();
    void cloneKit();
    void removeKit();
    void makeDefaultKit();
    void updateState();

    void scrollToSelectedKit();

    void apply() final;

public:
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_delButton;
    QPushButton m_makeDefaultButton;
    QPushButton m_filterButton;
    QPushButton m_defaultFilterButton;

    KitModel m_model;
    GroupedView m_groupedView{m_model};
    KitManagerConfigWidget m_configWidget;
};

KitOptionsPageWidget::KitOptionsPageWidget()
{
    m_addButton.setText(Tr::tr("Add"));
    m_cloneButton.setText(Tr::tr("Clone"));
    m_delButton.setText(Tr::tr("Remove"));
    m_makeDefaultButton.setText(Tr::tr("Make Default"));
    m_filterButton.setText(Tr::tr("Settings Filter..."));
    m_filterButton.setToolTip(Tr::tr("Choose which settings to display for this kit."));
    m_defaultFilterButton.setText(Tr::tr("Default Settings Filter..."));
    m_defaultFilterButton.setToolTip(Tr::tr("Choose which kit settings to display by default."));

    connect(&m_model, &Internal::KitModel::kitStateChanged,
            this, &KitOptionsPageWidget::updateState);

    using namespace Layouting;
    Column {
        Row {
            m_groupedView.view(),
            Column {
                noMargin,
                m_addButton,
                m_cloneButton,
                m_delButton,
                m_makeDefaultButton,
                m_filterButton,
                m_defaultFilterButton,
                st,
            }
        }
        // KitAspects will fill in here.
    }.attachTo(this);

    layout()->addWidget(&m_configWidget);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitOptionsPageWidget::updateState);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitOptionsPageWidget::updateState);
    connect(KitManager::instance(), &KitManager::kitUpdated, this, [this](Kit *k) {
        const int row = m_model.rowForOriginalKit(k);
        const int currentRow = m_groupedView.currentRow();
        if (row == currentRow && currentRow >= 0) {
            const KitData d = m_model.item(currentRow);
            m_configWidget.load(d.kit, d.workingCopy, d.hasUniqueName);
            emit m_configWidget.isAutoDetectedChanged();
        }
        updateState();
    });

    // Connect the dirty signal: sync working copy and update model display
    connect(&m_configWidget, &KitManagerConfigWidget::dirty, this, [this] {
        const int currentRow = m_groupedView.currentRow();
        if (currentRow < 0)
            return;
        Kit *wc = m_model.workingCopyForRow(currentRow);
        if (wc)
            wc->copyFrom(m_configWidget.workingCopy());
        m_model.setChanged(currentRow, true);
        m_model.notifyRowChanged(currentRow);
        markSettingsDirty();
    });
    connect(&m_configWidget, &KitManagerConfigWidget::isAutoDetectedChanged, this, [this] {
        const int currentRow = m_groupedView.currentRow();
        if (currentRow >= 0)
            m_model.notifyRowChanged(currentRow);
    });

    connect(&m_addButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::addNewKit);
    connect(&m_cloneButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::cloneKit);
    connect(&m_delButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::removeKit);
    connect(&m_makeDefaultButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::makeDefaultKit);
    connect(&m_filterButton, &QAbstractButton::clicked, this, [this] {
        QTC_ASSERT(m_groupedView.currentRow() >= 0, return);
        FilterKitAspectsDialog dlg(m_configWidget.workingCopy(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_configWidget.workingCopy()->setIrrelevantAspects(dlg.irrelevantAspects());
            m_configWidget.updateVisibility();
        }
    });
    connect(&m_defaultFilterButton, &QAbstractButton::clicked, this, [this] {
        FilterKitAspectsDialog dlg(nullptr, this);
        if (dlg.exec() == QDialog::Accepted) {
            KitManager::setIrrelevantAspects(dlg.irrelevantAspects());
            m_configWidget.updateVisibility();
        }
    });

    scrollToSelectedKit();

    updateState();
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
    const int currentRow = m_groupedView.currentRow();
    if (currentRow >= 0) {
        Kit *wc = m_model.workingCopyForRow(currentRow);
        if (wc)
            m_configWidget.save(wc);
    }

    m_model.apply();
    updateState();
}

void KitOptionsPageWidget::kitSelectionChanged(int oldRow, int newRow)
{
    // Save current widget state to working copy before switching
    if (oldRow >= 0) {
        Kit *wc = m_model.workingCopyForRow(oldRow);
        if (wc)
            m_configWidget.save(wc);
    }

    if (newRow >= 0) {
        const KitData d = m_model.item(newRow);
        m_configWidget.load(d.kit, d.workingCopy, d.hasUniqueName);
        m_configWidget.setVisible(true);
        m_groupedView.scrollToRow(newRow);
    } else {
        m_configWidget.setVisible(false);
    }

    updateState();
}

void KitOptionsPageWidget::addNewKit()
{
    Kit *k = m_model.markForAddition(nullptr);

    const int row = m_model.rowForKit(k);
    m_groupedView.selectRow(row);

    if (m_groupedView.currentRow() >= 0)
        m_configWidget.setFocusToName();

    markSettingsDirty();
}

Kit *KitOptionsPageWidget::currentKit() const
{
    const int row = m_groupedView.currentRow();
    return row >= 0 ? m_model.workingCopyForRow(row) : nullptr;
}

void KitOptionsPageWidget::cloneKit()
{
    Kit *current = currentKit();
    if (!current)
        return;

    Kit *k = m_model.markForAddition(current);
    const int row = m_model.rowForKit(k);
    m_groupedView.scrollToRow(row);
    m_groupedView.selectRow(row);

    if (m_groupedView.currentRow() >= 0)
        m_configWidget.setFocusToName();

    markSettingsDirty();
}

void KitOptionsPageWidget::removeKit()
{
    if (Kit *k = currentKit())
        m_model.markForRemoval(k);

    markSettingsDirty();
}

void KitOptionsPageWidget::makeDefaultKit()
{
    m_model.setVolatileDefaultRow(m_groupedView.currentRow());
    updateState();

    markSettingsDirty();
}

void KitOptionsPageWidget::updateState()
{
    bool canCopy = false;
    bool canDelete = false;
    bool canMakeDefault = false;

    if (Kit *k = currentKit()) {
        canCopy = true;
        canDelete = !k->detectionSource().isSdkProvided();
        const int row = m_model.rowForKit(k);
        canMakeDefault = !m_model.isDefaultKit(k) && row >= 0 && !m_model.isRemoved(row);
    }

    m_cloneButton.setEnabled(canCopy);
    m_delButton.setEnabled(canDelete);
    m_makeDefaultButton.setEnabled(canMakeDefault);
    m_filterButton.setEnabled(canCopy);
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

#include "kitoptionspage.moc"
