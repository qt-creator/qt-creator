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
    explicit KitManagerConfigWidget(Kit *k, bool &isDefaultKit, bool &hasUniqueName) :
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
    bool &m_isDefaultKit;
    bool m_fixingKit = false;
    bool &m_hasUniqueName;
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
    emit dirty();
}

void KitManagerConfigWidget::discard()
{
    if (m_kit) {
        m_modifiedKit->copyFrom(m_kit);
        m_isDefaultKit = (m_kit == KitManager::defaultKit());
    } else {
        // This branch will only ever get reached once during setup of widget for a not-yet-existing
        // kit.
        m_isDefaultKit = false;
    }
    m_iconButton->setIcon(m_modifiedKit->icon());
    m_nameEdit->setText(m_modifiedKit->unexpandedDisplayName());
    m_cachedDisplayName.clear();
    m_fileSystemFriendlyNameLineEdit->setText(m_modifiedKit->customFileSystemFriendlyName());
    emit dirty();
}

bool KitManagerConfigWidget::isDirty() const
{
    return !m_kit
            || !m_kit->isEqual(m_modifiedKit.get())
            || m_isDefaultKit != (KitManager::defaultKit() == m_kit);
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

//
// KitNode
//

class KitNode : public TreeItem
{
public:
    KitNode(Kit *k, KitModel *m, IOptionsPageWidget *pageWidget)
        : m_kit(k), m_model(m), m_pageWidget(pageWidget)
    {}

    ~KitNode() override { delete m_widget; }

    Kit *kit() const
    {
        if (m_widget) {
            // This is initially the same as m_kit, but potentially gets updated by
            // "apply" actions. See QTCREATORBUG-33829.
            return m_widget->kit();
        }
        return m_kit;
    }

    QVariant data(int, int role) const override
    {
        if (role == Qt::FontRole) {
            QFont f = QApplication::font();
            if (isDirty())
                f.setBold(!f.bold());
            if (isDefaultKit())
                f.setItalic(f.style() != QFont::StyleItalic);
            return f;
        }
        if (role == Qt::DisplayRole) {
            QString baseName = displayName();
            if (isDefaultKit())
                //: Mark up a kit as the default one.
                baseName = Tr::tr("%1 (default)").arg(baseName);
            return baseName;
        }

        if (role == Qt::DecorationRole)
            return displayIcon();

        if (role == Qt::ToolTipRole)
            return widget()->validityMessage();

        return {};
    }

    bool isDirty() const
    {
        if (m_widget)
            return m_widget->isDirty();
        return false;
    }

    QIcon displayIcon() const
    {
        if (m_widget)
            return m_widget->displayIcon();
        QTC_ASSERT(m_kit, return {});
        return m_kit->displayIcon();
    }

    QString displayName() const
    {
        if (m_widget)
            return m_widget->displayName();
        QTC_ASSERT(m_kit, return {});
        return m_kit->displayName();
    }

    bool isDefaultKit() const
    {
        return m_isDefaultKit;
    }

    bool isRegistering() const
    {
        if (m_widget)
            return m_widget->isRegistering();
        return false;
    }

    void setIsDefaultKit(bool on)
    {
        if (m_isDefaultKit == on)
            return;
        m_isDefaultKit = on;
        if (m_widget)
            emit m_widget->dirty();
    }

    KitManagerConfigWidget *widget() const
    {
        const_cast<KitNode *>(this)->ensureWidget();
        return m_widget;
    }

    void setHasUniqueName(bool on)
    {
        m_hasUniqueName = on;
        update();
    }

    void ensureWidget();

private:
    Kit * const m_kit;
    KitModel * const m_model;
    KitManagerConfigWidget *m_widget = nullptr;
    Core::IOptionsPageWidget * const m_pageWidget;
    bool m_isDefaultKit = false;
    bool m_hasUniqueName = true;
};

// KitModel

class KitModel : public TreeModel<TreeItem, TreeItem, KitNode>
{
    Q_OBJECT

public:
    explicit KitModel(IOptionsPageWidget *pageWidget);

    Kit *kit(const QModelIndex &) const;
    KitNode *kitNode(const QModelIndex &) const;
    QModelIndex indexOf(Kit *k) const;
    QModelIndex indexOf(Id kitId) const;

    void setDefaultKit(const QModelIndex &index);
    bool isDefaultKit(Kit *k) const;

    KitManagerConfigWidget *widget(const QModelIndex &);

    void apply();

    void markForRemoval(Kit *k);
    Kit *markForAddition(Kit *baseKit);

    void updateVisibility();

    QString newKitName(const QString &sourceName) const;

    void handleWidgetConstructionStart() { m_widgetConstructionGuard.lock(); }
    void handleWidgetConstructionEnd() { m_widgetConstructionGuard.unlock(); }

signals:
    void kitStateChanged();

private:
    void initializeFromKitManager();
    void addKit(Kit *k);
    void updateKit(Kit *k);
    void removeKit(Kit *k);
    void changeDefaultKit();
    void validateKitNames();

    KitNode *findWorkingCopy(Kit *k) const;
    KitNode *createNode(Kit *k);
    void setDefaultNode(KitNode *node);

    TreeItem *m_autoRoot;
    TreeItem *m_manualRoot;

    QList<KitNode *> m_toRemoveList;

    IOptionsPageWidget * const m_pageWidget;
    KitNode *m_defaultNode = nullptr;
    Guard m_widgetConstructionGuard;
};

KitModel::KitModel(IOptionsPageWidget *pageWidget)
    : m_pageWidget(pageWidget)
{
    setHeader(QStringList(Tr::tr("Name")));
    m_autoRoot = new StaticTreeItem({ProjectExplorer::Constants::msgAutoDetected()},
                                    {ProjectExplorer::Constants::msgAutoDetectedToolTip()});
    m_manualRoot = new StaticTreeItem(ProjectExplorer::Constants::msgManual());
    rootItem()->appendChild(m_autoRoot);
    rootItem()->appendChild(m_manualRoot);

    if (KitManager::isLoaded()) {
        for (Kit *k : KitManager::sortedKits())
            addKit(k);
        changeDefaultKit();
    }

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

Kit *KitModel::kit(const QModelIndex &index) const
{
    KitNode *n = kitNode(index);
    return n ? n->widget()->workingCopy() : nullptr;
}

KitNode *KitModel::kitNode(const QModelIndex &index) const
{
    TreeItem *n = itemForIndex(index);
    return (n && n->level() == 2) ? static_cast<KitNode *>(n) : nullptr;
}

QModelIndex KitModel::indexOf(Id kitId) const
{
    KitNode *n = findItemAtLevel<2>([kitId](KitNode *n) { return n->kit()->id() == kitId; });
    return n ? indexForItem(n) : QModelIndex();
}

QModelIndex KitModel::indexOf(Kit *k) const
{
    KitNode *n = findWorkingCopy(k);
    return n ? indexForItem(n) : QModelIndex();
}

void KitModel::setDefaultKit(const QModelIndex &index)
{
    if (KitNode *n = kitNode(index))
        setDefaultNode(n);
}

bool KitModel::isDefaultKit(Kit *k) const
{
    return m_defaultNode && m_defaultNode->widget()->workingCopy() == k;
}

KitManagerConfigWidget *KitModel::widget(const QModelIndex &index)
{
    KitNode *n = kitNode(index);
    return n ? n->widget() : nullptr;
}

void KitModel::validateKitNames()
{
    QHash<QString, int> nameHash;
    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->displayName();
        if (nameHash.contains(displayName))
            ++nameHash[displayName];
        else
            nameHash.insert(displayName, 1);
    });

    forItemsAtLevel<2>([&nameHash](KitNode *n) {
        const QString displayName = n->displayName();
        n->setHasUniqueName(nameHash.value(displayName) == 1);
    });
}

void KitModel::apply()
{
    emit layoutAboutToBeChanged();

    // Add/update dirty nodes before removing kits. This ensures the right kit ends up as default.
    forItemsAtLevel<2>([](KitNode *n) {
        if (n->isDirty()) {
            n->widget()->apply();
            n->update();
        }
    });

    // Remove unused kits:
    const QList<KitNode *> removeList = m_toRemoveList;
    for (KitNode *n : removeList)
        KitManager::deregisterKit(n->kit());

    emit layoutChanged(); // Force update.
}

void KitModel::markForRemoval(Kit *k)
{
    KitNode *node = findWorkingCopy(k);
    if (!node)
        return;

    if (node == m_defaultNode) {
        TreeItem *newDefault = m_autoRoot->firstChild();
        if (!newDefault)
            newDefault = m_manualRoot->firstChild();
        setDefaultNode(static_cast<KitNode *>(newDefault));
    }

    if (node == m_defaultNode)
        setDefaultNode(findItemAtLevel<2>([node](KitNode *kn) { return kn != node; }));

    takeItem(node);
    if (node->kit() == nullptr)
        delete node;
    else
        m_toRemoveList.append(node);
    validateKitNames();
}

Kit *KitModel::markForAddition(Kit *baseKit)
{
    const QString newName = newKitName(baseKit ? baseKit->unexpandedDisplayName() : QString());
    KitNode *node = createNode(nullptr);
    node->ensureWidget();
    m_manualRoot->appendChild(node);
    Kit *k = node->widget()->workingCopy();
    KitGuard g(k);
    if (baseKit) {
        k->copyFrom(baseKit);
        k->setDetectionSource(DetectionSource::Manual);
    } else {
        k->setup();
    }
    node->widget()->clearCachedDisplayName();
    k->setUnexpandedDisplayName(newName);

    if (!m_defaultNode)
        setDefaultNode(node);

    return k;
}

void KitModel::updateVisibility()
{
    forItemsAtLevel<2>([](const TreeItem *ti) {
        static_cast<const KitNode *>(ti)->widget()->updateVisibility();
    });
}

QString KitModel::newKitName(const QString &sourceName) const
{
    QList<Kit *> allKits;
    forItemsAtLevel<2>([&allKits](const TreeItem *ti) {
        allKits << static_cast<const KitNode *>(ti)->widget()->workingCopy();
    });
    return Kit::newKitName(sourceName, allKits);
}

KitNode *KitModel::findWorkingCopy(Kit *k) const
{
    return findItemAtLevel<2>([k](KitNode *n) { return n->widget()->workingCopy() == k; });
}

KitNode *KitModel::createNode(Kit *k)
{
    auto node = new KitNode(k, this, m_pageWidget);
    return node;
}

void KitModel::setDefaultNode(KitNode *node)
{
    if (m_defaultNode) {
        m_defaultNode->setIsDefaultKit(false);
        m_defaultNode->update();
    }
    m_defaultNode = node;
    if (m_defaultNode) {
        m_defaultNode->setIsDefaultKit(true);
        m_defaultNode->update();
    }
}

void KitModel::addKit(Kit *k)
{
    for (TreeItem *n : *m_manualRoot) {
        // Was added by us
        if (static_cast<KitNode *>(n)->isRegistering())
            return;
    }

    TreeItem *parent = k->detectionSource().isAutoDetected() ? m_autoRoot : m_manualRoot;
    parent->appendChild(createNode(k));

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
    QList<KitNode *> nodes = m_toRemoveList;
    for (KitNode *n : std::as_const(nodes)) {
        if (n->kit() == k) {
            m_toRemoveList.removeOne(n);
            if (m_defaultNode == n)
                m_defaultNode = nullptr;
            delete n;
            validateKitNames();
            return;
        }
    }

    KitNode *node = findItemAtLevel<2>([k](KitNode *n) {
        return n->kit() == k;
    });

    if (node == m_defaultNode)
        setDefaultNode(findItemAtLevel<2>([node](KitNode *kn) { return kn != node; }));

    destroyItem(node);

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::changeDefaultKit()
{
    Kit *defaultKit = KitManager::defaultKit();
    KitNode *node = findItemAtLevel<2>([defaultKit](KitNode *n) {
        return n->kit() == defaultKit;
    });
    setDefaultNode(node);
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

public:
    QTreeView m_kitsView;
    QPushButton m_addButton;
    QPushButton m_cloneButton;
    QPushButton m_delButton;
    QPushButton m_makeDefaultButton;
    QPushButton m_filterButton;
    QPushButton m_defaultFilterButton;

    KitModel m_model{this};
    KitSettingsSortModel m_sortModel;

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

    m_sortModel.setSortedCategories({Constants::msgAutoDetected(), Constants::msgManual()});
    m_sortModel.setSourceModel(&m_model);

    m_kitsView.setModel(&m_sortModel);
    m_kitsView.header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_kitsView.expandAll();
    m_kitsView.setSortingEnabled(true);
    m_kitsView.sortByColumn(0, Qt::AscendingOrder);

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

    setUseDirtyHook(false);
}

void KitOptionsPageWidget::scrollToSelectedKit()
{
    QModelIndex index = m_sortModel.mapFromSource(
        m_model.indexOf(Core::preselectedOptionsPageItem(Constants::KITS_SETTINGS_PAGE_ID)));
    m_kitsView.selectionModel()->select(index, QItemSelectionModel::Clear
                                             | QItemSelectionModel::SelectCurrent
                                             | QItemSelectionModel::Rows);
    m_kitsView.scrollTo(index);
}

void KitOptionsPageWidget::kitSelectionChanged()
{
    QModelIndex current = currentIndex();
    KitManagerConfigWidget * const newWidget = m_model.widget(m_sortModel.mapToSource(current));
    if (newWidget == m_currentWidget)
        return;

    if (m_currentWidget)
        m_currentWidget->setVisible(false);

    m_currentWidget = newWidget;

    if (m_currentWidget) {
        m_currentWidget->setVisible(true);
        m_kitsView.scrollTo(current);
    }

    updateState();
}

void KitOptionsPageWidget::addNewKit()
{
    Kit *k = m_model.markForAddition(nullptr);

    QModelIndex newIdx = m_sortModel.mapFromSource(m_model.indexOf(k));
    m_kitsView.selectionModel()->select(newIdx, QItemSelectionModel::Clear
                                              | QItemSelectionModel::SelectCurrent
                                              | QItemSelectionModel::Rows);

    if (m_currentWidget)
        m_currentWidget->setFocusToName();

    markSettingsDirty();
}

Kit *KitOptionsPageWidget::currentKit() const
{
    return m_model.kit(m_sortModel.mapToSource(currentIndex()));
}

void KitOptionsPageWidget::cloneKit()
{
    Kit *current = currentKit();
    if (!current)
        return;

    Kit *k = m_model.markForAddition(current);
    QModelIndex newIdx = m_sortModel.mapFromSource(m_model.indexOf(k));
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
    m_model.setDefaultKit(m_sortModel.mapToSource(currentIndex()));
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
        canMakeDefault = !m_model.isDefaultKit(k);
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

void KitNode::ensureWidget()
{
    if (m_widget)
        return;

    m_model->handleWidgetConstructionStart();

    m_widget = new KitManagerConfigWidget(m_kit, m_isDefaultKit, m_hasUniqueName);

    QObject::connect(m_widget, &KitManagerConfigWidget::dirty, m_model, [this] {
        update();
        markSettingsDirty();
    });

    QObject::connect(m_widget, &KitManagerConfigWidget::isAutoDetectedChanged, m_model, [this] {
        TreeItem *oldParent = parent();
        TreeItem *newParent =
            m_model->rootItem()->childAt(m_widget->workingCopy()->detectionSource().isAutoDetected() ? 0 : 1);
        if (oldParent && oldParent != newParent) {
            m_model->takeItem(this);
            newParent->appendChild(this);
        }
    });

    m_pageWidget->layout()->addWidget(m_widget);

    m_model->handleWidgetConstructionEnd();
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
