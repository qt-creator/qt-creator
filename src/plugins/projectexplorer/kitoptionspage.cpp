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
#include <QHeaderView>
#include <QItemSelectionModel>
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
    Kit *kit = nullptr;
    KitManagerConfigWidget *widget = nullptr;  // lazy; excluded from operator==
    bool hasUniqueName = true;

    bool operator==(const KitData &other) const
    {
        return kit == other.kit
            && hasUniqueName == other.hasUniqueName;
    }
};

} // namespace ProjectExplorer::Internal

Q_DECLARE_METATYPE(ProjectExplorer::Internal::KitData)

namespace ProjectExplorer {

bool KitSettingsSortModel::lessThan(const QModelIndex &source_left,
                                    const QModelIndex &source_right) const
{
    const auto defaultCmp = [&] { return SortModel::lessThan(source_left, source_right); };

    if (m_sortedCategories.isEmpty() || source_left.parent().isValid())
        return defaultCmp();

    QTC_ASSERT(!source_right.parent().isValid(), return defaultCmp());
    const int leftIndex = m_sortedCategories.indexOf(sourceModel()->data(source_left));
    QTC_ASSERT(leftIndex != -1, return defaultCmp());
    if (leftIndex == 0)
        return true;
    const int rightIndex = m_sortedCategories.indexOf(sourceModel()->data(source_right));
    QTC_ASSERT(rightIndex != -1, return defaultCmp());
    return leftIndex < rightIndex;
}

namespace Internal {

class KitManagerConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KitManagerConfigWidget(Kit *k, bool isDefaultKit, bool hasUniqueName) :
        m_iconButton(new QToolButton),
        m_nameEdit(new QLineEdit),
        m_fileSystemFriendlyNameLineEdit(new QLineEdit),
        m_kit(k),
        m_modifiedKit(std::make_unique<Kit>(Id(WORKING_COPY_KIT_ID))),
        m_isDefaultKit(isDefaultKit),
        m_hasUniqueName(hasUniqueName)
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
        Q_ASSERT(fileSystemFriendlyNameRegexp.isValid());
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

        discard();

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
        connect(km, &KitManager::kitUpdated,
                this, &KitManagerConfigWidget::kitWasUpdated);

        auto chooser = new VariableChooser(this);
        chooser->addSupportedWidget(m_nameEdit);
        chooser->addMacroExpanderProvider({this, [this] { return m_modifiedKit->macroExpander(); }});

        addAspectsToWorkingCopy(page);

        page.attachTo(this);

        updateVisibility();

        if (k && k->detectionSource().isAutoDetected())
            makeStickySubWidgetsReadOnly();
        setVisible(false);
    }

    ~KitManagerConfigWidget() override
    {
        qDeleteAll(m_kitAspects);
        m_kitAspects.clear();

        // Make sure our workingCopy did not get registered somehow:
        QTC_CHECK(!contains(KitManager::kits(), equal(&Kit::id, Id(WORKING_COPY_KIT_ID))));
    }

    QString displayName() const;
    QIcon displayIcon() const;
    void clearCachedDisplayName(); // FIXME: Remove cached name?

    void setFocusToName();
    void apply();
    void discard();
    bool isDirty() const;
    QString validityMessage() const;
    void addAspectsToWorkingCopy(Layouting::Layout &parent);
    void makeStickySubWidgetsReadOnly();

    Kit *kit() const { return m_kit; }
    Kit *workingCopy() const;
    bool isRegistering() const { return m_isRegistering; }
    bool isDefaultKit() const;
    void setIsDefaultKit(bool on) { m_isDefaultKit = on; }
    void setHasUniqueName(bool on) { m_hasUniqueName = on; }
    void updateVisibility();

signals:
    void dirty();
    void isAutoDetectedChanged();

private:
    void setIcon();
    void resetIcon();
    void setDisplayName();
    void setFileSystemFriendlyName();
    void workingCopyWasUpdated(ProjectExplorer::Kit *k);
    void kitWasUpdated(ProjectExplorer::Kit *k);

    enum LayoutColumns {
        LabelColumn,
        WidgetColumn,
        ButtonColumn
    };

    void showEvent(QShowEvent *event) override;

    QToolButton *m_iconButton;
    QLineEdit *m_nameEdit;
    QLineEdit *m_fileSystemFriendlyNameLineEdit;
    QList<KitAspect *> m_kitAspects;
    Kit *m_kit;
    std::unique_ptr<Kit> m_modifiedKit;
    bool m_isDefaultKit;
    bool m_fixingKit = false;
    bool m_hasUniqueName;
    bool m_isRegistering = false;
    mutable QString m_cachedDisplayName;
};


QString KitManagerConfigWidget::displayName() const
{
    if (m_cachedDisplayName.isEmpty())
        m_cachedDisplayName = m_modifiedKit->displayName();
    return m_cachedDisplayName;
}

QIcon KitManagerConfigWidget::displayIcon() const
{
    // Special case: Extra warning if there are no errors but name is not unique.
    if (m_modifiedKit->isValid() && !m_hasUniqueName) {
        static const QIcon warningIcon(Icons::WARNING.icon());
        return warningIcon;
    }

    return m_modifiedKit->displayIcon();
}

void KitManagerConfigWidget::clearCachedDisplayName()
{
    m_cachedDisplayName.clear();
}

void KitManagerConfigWidget::setFocusToName()
{
    m_nameEdit->selectAll();
    m_nameEdit->setFocus();
}

void KitManagerConfigWidget::apply()
{
    // TODO: Rework the mechanism so this won't be necessary.
    const bool wasDefaultKit = m_isDefaultKit;

    for (KitAspect *kitAspect : m_kitAspects)
        kitAspect->apply();

    const auto copyIntoKit = [this](Kit *k) { k->copyFrom(m_modifiedKit.get()); };
    if (m_kit) {
        copyIntoKit(m_kit);
        KitManager::notifyAboutUpdate(m_kit);
    } else {
        m_isRegistering = true;
        m_kit = KitManager::registerKit(copyIntoKit);
        m_isRegistering = false;
    }
    m_isDefaultKit = wasDefaultKit;
    if (m_isDefaultKit)
        KitManager::setDefaultKit(m_kit);
}

void KitManagerConfigWidget::discard()
{
    if (m_kit)
        m_modifiedKit->copyFrom(m_kit);

    m_iconButton->setIcon(m_modifiedKit->icon());
    m_nameEdit->setText(m_modifiedKit->unexpandedDisplayName());
    m_cachedDisplayName.clear();
    m_fileSystemFriendlyNameLineEdit->setText(m_modifiedKit->customFileSystemFriendlyName());

    for (KitAspect *kitAspect : m_kitAspects)
        kitAspect->cancel();
}

bool KitManagerConfigWidget::isDirty() const
{
    if (!m_kit)
        return true;

    if (!m_kit->isEqual(m_modifiedKit.get()))
        return true;

    if (m_isDefaultKit != (KitManager::defaultKit() == m_kit))
        return true;

    for (KitAspect *kitAspect : m_kitAspects) {
        if (kitAspect->isDirty())
            return true;
    }

    return false;
}

QString KitManagerConfigWidget::validityMessage() const
{
    Tasks tmp;
    if (!m_hasUniqueName)
        tmp.append(CompileTask(Task::Warning, Tr::tr("Display name is not unique.")));

    return m_modifiedKit->toHtml(tmp);
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
            this, &KitManagerConfigWidget::dirty);
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
        aspect->setVisible(m_modifiedKit->isAspectRelevant(aspect->factory()->id()));
}

void KitManagerConfigWidget::makeStickySubWidgetsReadOnly()
{
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->makeStickySubWidgetsReadOnly();
}

Kit *KitManagerConfigWidget::workingCopy() const
{
    return m_modifiedKit.get();
}

bool KitManagerConfigWidget::isDefaultKit() const
{
    return m_isDefaultKit;
}

void KitManagerConfigWidget::setIcon()
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(m_modifiedKit.get());
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
                                                 m_modifiedKit->setDeviceTypeForIcon(
                                                     factory->deviceType());
                                                 emit dirty();
                                             });
        action->setIconVisibleInMenu(true);
    }
    iconMenu.addSeparator();
    iconMenu.addAction(PathChooser::browseButtonLabel(), [this] {
        const FilePath path = FileUtils::getOpenFilePath(Tr::tr("Select Icon"),
                                                         m_modifiedKit->iconPath(),
                                                         Tr::tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return;
        const QIcon icon(path.toUrlishString());
        if (icon.isNull())
            return;
        m_iconButton->setIcon(icon);
        m_modifiedKit->setIconPath(path);
        emit dirty();
    });
    iconMenu.exec(mapToGlobal(m_iconButton->pos()));
}

void KitManagerConfigWidget::resetIcon()
{
    m_modifiedKit->setIconPath({});
    emit dirty();
    markSettingsDirty();
}

void KitManagerConfigWidget::setDisplayName()
{
    int pos = m_nameEdit->cursorPosition();
    m_cachedDisplayName.clear();
    m_modifiedKit->setUnexpandedDisplayName(m_nameEdit->text());
    m_nameEdit->setCursorPosition(pos);
    markSettingsDirty();
}

void KitManagerConfigWidget::setFileSystemFriendlyName()
{
    if (m_fileSystemFriendlyNameLineEdit->text() == m_modifiedKit->customFileSystemFriendlyName())
        return;
    const int pos = m_fileSystemFriendlyNameLineEdit->cursorPosition();
    m_modifiedKit->setCustomFileSystemFriendlyName(m_fileSystemFriendlyNameLineEdit->text());
    m_fileSystemFriendlyNameLineEdit->setCursorPosition(pos);
    markSettingsDirty();
}

void KitManagerConfigWidget::workingCopyWasUpdated(Kit *k)
{
    if (k != m_modifiedKit.get() || m_fixingKit)
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

void KitManagerConfigWidget::kitWasUpdated(Kit *k)
{
    if (m_kit == k) {
        bool emitSignal = m_kit->detectionSource().isAutoDetected()
                          != m_modifiedKit->detectionSource().isAutoDetected();
        discard();
        if (emitSignal)
            emit isAutoDetectedChanged();
    }
    updateVisibility();
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
    explicit KitModel(IOptionsPageWidget *pageWidget);

    Kit *kit(int row) const;
    int rowForKit(Kit *k) const;
    int rowForId(Id kitId) const;
    bool isDefaultKit(Kit *k) const;
    KitManagerConfigWidget *widgetForRow(int row);

    void apply() final;

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

    void updateVisibility();

    QString newKitName(const QString &sourceName) const;

    void handleWidgetConstructionStart() { m_widgetConstructionGuard.lock(); }
    void handleWidgetConstructionEnd() { m_widgetConstructionGuard.unlock(); }

signals:
    void kitStateChanged();

private:
    void onDefaultRowChanged(int oldRow, int newRow) override;
    QVariant variantData(const QVariant &v, int column, int role) const final;
    void addKit(Kit *k);
    void updateKit(Kit *k);
    void removeKit(Kit *k);
    void changeDefaultKit();
    void validateKitNames();

    IOptionsPageWidget * const m_pageWidget;
    Guard m_widgetConstructionGuard;
};

KitModel::KitModel(IOptionsPageWidget *pageWidget)
    : m_pageWidget(pageWidget)
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

QVariant KitModel::variantData(const QVariant &v, int /*column*/, int role) const
{
    const KitData d = fromVariant(v);
    switch (role) {
    case Qt::DisplayRole: {
        if (d.widget)
            return d.widget->displayName();
        if (d.kit)
            return d.kit->displayName();
        return {};
    }
    case Qt::DecorationRole:
        if (d.widget)
            return d.widget->displayIcon();
        if (d.kit)
            return d.kit->displayIcon();
        return {};
    case Qt::ToolTipRole:
        if (d.widget)
            return d.widget->validityMessage();
        return {};
    default:
        return {};
    }
}

Kit *KitModel::kit(int row) const
{
    const KitData d = item(row);
    return d.widget ? d.widget->workingCopy() : nullptr;
}

int KitModel::rowForKit(Kit *k) const
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.widget && d.widget->workingCopy() == k)
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
        if (d.widget && d.widget->workingCopy()->id() == kitId)
            return row;
    }
    return -1;
}

void KitModel::onDefaultRowChanged(int oldRow, int newRow)
{
    if (oldRow >= 0 && oldRow < itemCount()) {
        if (const KitData d = item(oldRow); d.widget)
            d.widget->setIsDefaultKit(false);
    }
    if (newRow >= 0 && newRow < itemCount()) {
        if (const KitData d = item(newRow); d.widget)
            d.widget->setIsDefaultKit(true);
    }
}

bool KitModel::isDefaultKit(Kit *k) const
{
    const int row = rowForKit(k);
    return row >= 0 && isDefault(row);
}

KitManagerConfigWidget *KitModel::widgetForRow(int row)
{
    KitData d = item(row);
    if (d.widget)
        return d.widget;

    handleWidgetConstructionStart();

    auto widget = new KitManagerConfigWidget(d.kit, isDefault(row), d.hasUniqueName);

    QObject::connect(widget, &KitManagerConfigWidget::dirty, this, [this, widget] {
        const int r = rowForKit(widget->workingCopy());
        if (r < 0)
            return;
        setChanged(r, true);
        notifyRowChanged(r);
        markSettingsDirty();
    });
    QObject::connect(widget, &KitManagerConfigWidget::isAutoDetectedChanged, this, [this, widget] {
        const int r = rowForKit(widget->workingCopy());
        if (r >= 0)
            notifyRowChanged(r);
    });

    m_pageWidget->layout()->addWidget(widget);

    d.widget = widget;
    setVolatileItem(row, d);

    handleWidgetConstructionEnd();
    return widget;
}

void KitModel::apply()
{
    QList<Kit *> kitsToDeregister;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            const KitData d = item(row);
            if (d.kit)
                kitsToDeregister.append(d.kit);
        }
    }

    // Apply dirty widgets before removing kits to ensures right kit ends up as default.
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        KitData d = item(row);
        if (d.widget && (isAdded(row) || d.widget->isDirty())) {
            d.widget->apply();
            d.kit = d.widget->kit();
            setVolatileItem(row, d);
        }
    }

    GroupedModel::apply();

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

    markRemoved(row);
    validateKitNames();
    emit kitStateChanged();
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    const QString newName = newKitName(baseKit ? baseKit->unexpandedDisplayName() : QString());

    const int newRow = itemCount();
    appendVolatileItem(KitData{});

    KitManagerConfigWidget *w = widgetForRow(newRow);
    Kit *k = w->workingCopy();
    KitGuard g(k);
    if (baseKit) {
        k->copyFrom(baseKit);
        k->setDetectionSource(DetectionSource::Manual);
    } else {
        k->setup();
    }
    w->clearCachedDisplayName();
    k->setUnexpandedDisplayName(newName);

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

void KitModel::updateVisibility()
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.widget)
            d.widget->updateVisibility();
    }
}

QString KitModel::newKitName(const QString &sourceName) const
{
    QList<Kit *> allKits;
    for (int row = 0; row < itemCount(); ++row)
        if (Kit *k = kit(row))
            allKits << k;
    return Kit::newKitName(sourceName, allKits);
}

void KitModel::addKit(Kit *k)
{
    for (int row = 0; row < itemCount(); ++row) {
        const KitData d = item(row);
        if (d.kit && d.kit->detectionSource().isAutoDetected())
            continue;
        if (d.widget && d.widget->isRegistering()) {
            KitData dd = d;
            dd.kit = k;
            setVolatileItem(row, dd);
            return;
        }
    }

    KitData newData;
    newData.kit = k;
    appendVariant(toVariant(newData));
    if (k == KitManager::defaultKit())
        setDefaultRow(rowForId(k->id()));

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::updateKit(Kit *)
{
    if (m_widgetConstructionGuard.isLocked())
        return;

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
        if (isRemoved(row))
            continue;
        const KitData d = item(row);
        const QString name = d.widget ? d.widget->displayName()
                                      : (d.kit ? d.kit->displayName() : QString());
        if (!name.isEmpty())
            nameHash[name]++;
    }

    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        KitData d = item(row);
        const QString name = d.widget ? d.widget->displayName()
                                      : (d.kit ? d.kit->displayName() : QString());
        const bool unique = nameHash.value(name) == 1;
        if (d.hasUniqueName != unique) {
            d.hasUniqueName = unique;
            if (d.widget)
                d.widget->setHasUniqueName(unique);
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

    QModelIndex currentIndex() const;
    Kit *currentKit() const;

    void kitSelectionChanged();
    void addNewKit();
    void cloneKit();
    void removeKit();
    void makeDefaultKit();
    void updateState();

    void scrollToSelectedKit();

    void apply() final { m_model.apply(); }

    int mapToRow(const QModelIndex &viewIdx) const
    {
        return m_model.mapToSource(viewIdx).row();
    }

    QModelIndex mapFromRow(int row) const
    {
        if (row < 0)
            return {};
        return m_model.mapFromSource(m_model.index(row, 0));
    }

public:
    QTreeView m_kitsView;
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_delButton;
    QPushButton m_makeDefaultButton;
    QPushButton m_filterButton;
    QPushButton m_defaultFilterButton;

    KitModel m_model{this};

    KitManagerConfigWidget *m_currentWidget = nullptr;
};

KitOptionsPageWidget::KitOptionsPageWidget()
{
    m_kitsView.setUniformRowHeights(true);
    m_kitsView.header()->setStretchLastSection(true);
    m_kitsView.setSizePolicy(m_kitsView.sizePolicy().horizontalPolicy(),
                              QSizePolicy::Ignored);

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

    m_kitsView.setModel(m_model.groupedDisplayModel());
    m_kitsView.header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_kitsView.expandAll();
    m_kitsView.setSortingEnabled(true);
    m_kitsView.sortByColumn(0, Qt::AscendingOrder);

    connect(m_model.groupedDisplayModel(), &QAbstractItemModel::modelReset,
            &m_kitsView, &QTreeView::expandAll);

    using namespace Layouting;
    Column {
        Row {
            m_kitsView,
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

    connect(m_kitsView.selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &KitOptionsPageWidget::kitSelectionChanged);

    connect(&m_addButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::addNewKit);
    connect(&m_cloneButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::cloneKit);
    connect(&m_delButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::removeKit);
    connect(&m_makeDefaultButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::makeDefaultKit);
    connect(&m_filterButton, &QAbstractButton::clicked, this, [this] {
        QTC_ASSERT(m_currentWidget, return);
        FilterKitAspectsDialog dlg(m_currentWidget->workingCopy(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_currentWidget->workingCopy()->setIrrelevantAspects(dlg.irrelevantAspects());
            m_currentWidget->updateVisibility();
        }
    });
    connect(&m_defaultFilterButton, &QAbstractButton::clicked, this, [this] {
        FilterKitAspectsDialog dlg(nullptr, this);
        if (dlg.exec() == QDialog::Accepted) {
            KitManager::setIrrelevantAspects(dlg.irrelevantAspects());
            m_model.updateVisibility();
        }
    });

    scrollToSelectedKit();

    updateState();
}

void KitOptionsPageWidget::scrollToSelectedKit()
{
    const int row = m_model.rowForId(
        Core::preselectedOptionsPageItem(Constants::KITS_SETTINGS_PAGE_ID));
    QModelIndex index = mapFromRow(row);
    m_kitsView.selectionModel()->select(index, QItemSelectionModel::Clear
                                             | QItemSelectionModel::SelectCurrent
                                             | QItemSelectionModel::Rows);
    m_kitsView.scrollTo(index);
}

void KitOptionsPageWidget::kitSelectionChanged()
{
    const int row = mapToRow(currentIndex());
    KitManagerConfigWidget * const newWidget = row >= 0 ? m_model.widgetForRow(row) : nullptr;
    if (newWidget == m_currentWidget)
        return;

    if (m_currentWidget)
        m_currentWidget->setVisible(false);

    m_currentWidget = newWidget;

    if (m_currentWidget) {
        m_currentWidget->setVisible(true);
        m_kitsView.scrollTo(currentIndex());
    }

    updateState();
}

void KitOptionsPageWidget::addNewKit()
{
    Kit *k = m_model.markForAddition(nullptr);

    const int row = m_model.rowForKit(k);
    QModelIndex newIdx = mapFromRow(row);
    m_kitsView.selectionModel()->select(newIdx, QItemSelectionModel::Clear
                                              | QItemSelectionModel::SelectCurrent
                                              | QItemSelectionModel::Rows);

    if (m_currentWidget)
        m_currentWidget->setFocusToName();

    markSettingsDirty();
}

Kit *KitOptionsPageWidget::currentKit() const
{
    const int row = mapToRow(currentIndex());
    return row >= 0 ? m_model.kit(row) : nullptr;
}

void KitOptionsPageWidget::cloneKit()
{
    Kit *current = currentKit();
    if (!current)
        return;

    Kit *k = m_model.markForAddition(current);
    const int row = m_model.rowForKit(k);
    QModelIndex newIdx = mapFromRow(row);
    m_kitsView.scrollTo(newIdx);
    m_kitsView.selectionModel()->select(newIdx, QItemSelectionModel::Clear
                                              | QItemSelectionModel::SelectCurrent
                                              | QItemSelectionModel::Rows);

    if (m_currentWidget)
        m_currentWidget->setFocusToName();

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
    m_model.setVolatileDefaultRow(mapToRow(currentIndex()));
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

QModelIndex KitOptionsPageWidget::currentIndex() const
{
    QModelIndexList idxs = m_kitsView.selectionModel()->selectedRows();
    if (idxs.count() != 1)
        return {};
    return idxs.at(0);
}

// KitOptionsPage

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

const KitsSettingsPage theKitsSettingsPage;

} // Internal
} // ProjectExplorer

#include "kitoptionspage.moc"
