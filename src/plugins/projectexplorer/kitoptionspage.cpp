// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitoptionspage.h"

#include "filterkitaspectsdialog.h"
#include "kit.h"
#include "kitmanager.h"
#include "kitmanagerconfigwidget.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/id.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

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

// Page pre-selection

static Id selectedKitId;

void setSelectectKitId(const Id &kitId)
{
    selectedKitId = kitId;
}

class KitManagerConfigWidget;

class KitNode : public TreeItem
{
public:
    KitNode(Kit *k, KitModel *m, QBoxLayout *parentLayout)
        : m_kit(k), m_model(m), m_parentLayout(parentLayout)
    {}

    ~KitNode() override { delete m_widget; }

    Kit *kit() const { return m_kit; }

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
    }

private:
    void ensureWidget();

    Kit *m_kit = m_kit;
    KitModel *m_model = nullptr;
    KitManagerConfigWidget *m_widget = nullptr;
    QBoxLayout *m_parentLayout = nullptr;
    bool m_isDefaultKit = false;
    bool m_hasUniqueName = true;
};

// KitModel

class KitModel : public TreeModel<TreeItem, TreeItem, KitNode>
{
    Q_OBJECT

public:
    explicit KitModel(QBoxLayout *parentLayout, QObject *parent = nullptr);

    Kit *kit(const QModelIndex &);
    KitNode *kitNode(const QModelIndex &);
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

    QBoxLayout *m_parentLayout;
    KitNode *m_defaultNode = nullptr;
};

KitModel::KitModel(QBoxLayout *parentLayout, QObject *parent)
    : TreeModel<TreeItem, TreeItem, KitNode>(parent),
      m_parentLayout(parentLayout)
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

Kit *KitModel::kit(const QModelIndex &index)
{
    KitNode *n = kitNode(index);
    return n ? n->widget()->workingCopy() : nullptr;
}

KitNode *KitModel::kitNode(const QModelIndex &index)
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
    m_manualRoot->appendChild(node);
    Kit *k = node->widget()->workingCopy();
    KitGuard g(k);
    if (baseKit) {
        k->copyFrom(baseKit);
        k->setAutoDetected(false); // Make sure we have a manual kit!
        k->setSdkProvided(false);
    } else {
        k->setup();
    }
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
    auto node = new KitNode(k, this, m_parentLayout);
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

    TreeItem *parent = k->isAutoDetected() ? m_autoRoot : m_manualRoot;
    parent->appendChild(createNode(k));

    validateKitNames();
    emit kitStateChanged();
}

void KitModel::updateKit(Kit *)
{
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

    void apply() final { m_model->apply(); }

public:
    QTreeView *m_kitsView = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_cloneButton = nullptr;
    QPushButton *m_delButton = nullptr;
    QPushButton *m_makeDefaultButton = nullptr;
    QPushButton *m_filterButton = nullptr;
    QPushButton *m_defaultFilterButton = nullptr;

    KitModel *m_model = nullptr;
    KitSettingsSortModel *m_sortModel = nullptr;
    QItemSelectionModel *m_selectionModel = nullptr;
    KitManagerConfigWidget *m_currentWidget = nullptr;
};

KitOptionsPageWidget::KitOptionsPageWidget()
{
    m_kitsView = new QTreeView(this);
    m_kitsView->setUniformRowHeights(true);
    m_kitsView->header()->setStretchLastSection(true);
    m_kitsView->setSizePolicy(m_kitsView->sizePolicy().horizontalPolicy(),
                              QSizePolicy::Ignored);

    m_addButton = new QPushButton(Tr::tr("Add"), this);
    m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
    m_delButton = new QPushButton(Tr::tr("Remove"), this);
    m_makeDefaultButton = new QPushButton(Tr::tr("Make Default"), this);
    m_filterButton = new QPushButton(Tr::tr("Settings Filter..."), this);
    m_filterButton->setToolTip(Tr::tr("Choose which settings to display for this kit."));
    m_defaultFilterButton = new QPushButton(Tr::tr("Default Settings Filter..."), this);
    m_defaultFilterButton->setToolTip(Tr::tr("Choose which kit settings to display by default."));

    auto buttonLayout = new QVBoxLayout;
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addWidget(m_makeDefaultButton);
    buttonLayout->addWidget(m_filterButton);
    buttonLayout->addWidget(m_defaultFilterButton);
    buttonLayout->addStretch();

    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(m_kitsView);
    horizontalLayout->addLayout(buttonLayout);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(horizontalLayout);

    m_model = new Internal::KitModel(verticalLayout, this);
    connect(m_model, &Internal::KitModel::kitStateChanged,
            this, &KitOptionsPageWidget::updateState);
    verticalLayout->setStretch(0, 1);
    verticalLayout->setStretch(1, 0);
    m_sortModel = new KitSettingsSortModel(this);
    m_sortModel->setSortedCategories({Constants::msgAutoDetected(), Constants::msgManual()});
    m_sortModel->setSourceModel(m_model);

    m_kitsView->setModel(m_sortModel);
    m_kitsView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_kitsView->expandAll();
    m_kitsView->setSortingEnabled(true);
    m_kitsView->sortByColumn(0, Qt::AscendingOrder);

    m_selectionModel = m_kitsView->selectionModel();
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &KitOptionsPageWidget::kitSelectionChanged);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &KitOptionsPageWidget::kitSelectionChanged);

    // Set up add menu:
    connect(m_addButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::addNewKit);
    connect(m_cloneButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::cloneKit);
    connect(m_delButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::removeKit);
    connect(m_makeDefaultButton, &QAbstractButton::clicked,
            this, &KitOptionsPageWidget::makeDefaultKit);
    connect(m_filterButton, &QAbstractButton::clicked, this, [this] {
        QTC_ASSERT(m_currentWidget, return);
        FilterKitAspectsDialog dlg(m_currentWidget->workingCopy(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_currentWidget->workingCopy()->setIrrelevantAspects(dlg.irrelevantAspects());
            m_currentWidget->updateVisibility();
        }
    });
    connect(m_defaultFilterButton, &QAbstractButton::clicked, this, [this] {
        FilterKitAspectsDialog dlg(nullptr, this);
        if (dlg.exec() == QDialog::Accepted) {
            KitManager::setIrrelevantAspects(dlg.irrelevantAspects());
            m_model->updateVisibility();
        }
    });

    scrollToSelectedKit();

    updateState();
}

void KitOptionsPageWidget::scrollToSelectedKit()
{
    QModelIndex index = m_sortModel->mapFromSource(m_model->indexOf(selectedKitId));
    m_selectionModel->select(index,
                             QItemSelectionModel::Clear
                                 | QItemSelectionModel::SelectCurrent
                                 | QItemSelectionModel::Rows);
    m_kitsView->scrollTo(index);
}

void KitOptionsPageWidget::kitSelectionChanged()
{
    QModelIndex current = currentIndex();
    KitManagerConfigWidget * const newWidget = m_model->widget(m_sortModel->mapToSource(current));
    if (newWidget == m_currentWidget)
        return;

    if (m_currentWidget)
        m_currentWidget->setVisible(false);

    m_currentWidget = newWidget;

    if (m_currentWidget) {
        m_currentWidget->setVisible(true);
        m_kitsView->scrollTo(current);
    }

    updateState();
}

void KitOptionsPageWidget::addNewKit()
{
    Kit *k = m_model->markForAddition(nullptr);

    QModelIndex newIdx = m_sortModel->mapFromSource(m_model->indexOf(k));
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

Kit *KitOptionsPageWidget::currentKit() const
{
    return m_model->kit(m_sortModel->mapToSource(currentIndex()));
}

void KitOptionsPageWidget::cloneKit()
{
    Kit *current = currentKit();
    if (!current)
        return;

    Kit *k = m_model->markForAddition(current);
    QModelIndex newIdx = m_sortModel->mapFromSource(m_model->indexOf(k));
    m_kitsView->scrollTo(newIdx);
    m_selectionModel->select(newIdx,
                             QItemSelectionModel::Clear
                             | QItemSelectionModel::SelectCurrent
                             | QItemSelectionModel::Rows);
}

void KitOptionsPageWidget::removeKit()
{
    if (Kit *k = currentKit())
        m_model->markForRemoval(k);
}

void KitOptionsPageWidget::makeDefaultKit()
{
    m_model->setDefaultKit(m_sortModel->mapToSource(currentIndex()));
    updateState();
}

void KitOptionsPageWidget::updateState()
{
    if (!m_kitsView)
        return;

    bool canCopy = false;
    bool canDelete = false;
    bool canMakeDefault = false;

    if (Kit *k = currentKit()) {
        canCopy = true;
        canDelete = !k->isAutoDetected();
        canMakeDefault = !m_model->isDefaultKit(k);
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
    m_makeDefaultButton->setEnabled(canMakeDefault);
    m_filterButton->setEnabled(canCopy);
}

QModelIndex KitOptionsPageWidget::currentIndex() const
{
    if (!m_selectionModel)
        return {};

    QModelIndexList idxs = m_selectionModel->selectedRows();
    if (idxs.count() != 1)
        return {};
    return idxs.at(0);
}

void KitNode::ensureWidget()
{
    if (m_widget)
        return;

    m_widget = new KitManagerConfigWidget(m_kit, m_isDefaultKit, m_hasUniqueName);

    QObject::connect(m_widget, &KitManagerConfigWidget::dirty, m_model, [this] { update(); });

    QObject::connect(m_widget, &KitManagerConfigWidget::isAutoDetectedChanged, m_model, [this] {
        TreeItem *oldParent = parent();
        TreeItem *newParent =
            m_model->rootItem()->childAt(m_widget->workingCopy()->isAutoDetected() ? 0 : 1);
        if (oldParent && oldParent != newParent) {
            m_model->takeItem(this);
            newParent->appendChild(this);
        }
    });
    m_parentLayout->addWidget(m_widget);
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
        setDisplayCategory(Tr::tr("Kits"));
        setCategoryIconPath(":/projectexplorer/images/settingscategory_kits.png");
        setWidgetCreator([] { return new Internal::KitOptionsPageWidget; });
    }
};

const KitsSettingsPage theKitsSettingsPage;

} // Internal
} // ProjectExplorer

#include "kitoptionspage.moc"
