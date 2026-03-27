// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolchainoptionspage.h"

#include "abi.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/devicemanagermodel.h"
#include "kitaspect.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "toolchain.h"
#include "toolchainconfigwidget.h"
#include "toolchainmanager.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/groupedmodel.h>
#include <utils/guard.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QTextStream>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

using namespace Utils;

namespace ProjectExplorer::Internal {

QVariant toolchainBundleData(const std::optional<ToolchainBundle> &bundle, int column, int role)
{
    switch (role) {
    case Qt::DisplayRole:
        if (column == 0)
            return bundle ? bundle->displayName() : Tr::tr("None", "Toolchain bundle display name");
        if (!bundle)
            return {};
        if (column == 1)
            return bundle->typeDisplayName();
        if (column == 2 && bundle->factory())
            return ToolchainManager::displayNameOfLanguageCategory(bundle->factory()->languageCategory());
        return {};
    case Qt::ToolTipRole: {
        if (!bundle)
            return {};
        QString toolTip;
        const ToolchainBundle::Valid validity = bundle->validity();
        if (validity != ToolchainBundle::Valid::None) {
            toolTip = Tr::tr("<nobr><b>ABI:</b> %1").arg(bundle->targetAbi().toString());
            if (validity == ToolchainBundle::Valid::Some)
                toolTip.append("<br/>").append(Tr::tr("Not all compilers are set up correctly."));
        } else {
            toolTip = Tr::tr("This toolchain is invalid.");
        }
        return QVariant("<div style=\"white-space:pre\">" + toolTip + "</div>");
    }
    case Qt::DecorationRole:
        if (!bundle)
            return {};
        if (column == 0) {
            switch (bundle->validity()) {
            case ToolchainBundle::Valid::All:
                break;
            case ToolchainBundle::Valid::Some:
                return Utils::Icons::WARNING.icon();
            case ToolchainBundle::Valid::None:
                return Utils::Icons::CRITICAL.icon();
            }
        }
        return QVariant();
    case KitAspect::IdRole:
        return bundle ? bundle->bundleId().toSetting() : QVariant();
    case KitAspect::IsNoneRole:
        return !bundle;
    case KitAspect::TypeRole:
        return bundle ? bundle->typeDisplayName() : QString();
    case KitAspect::QualityRole:
        return bundle ? int(bundle->validity()) : -1;
    case FilePathRole:
        return bundle && bundle->validity() != ToolchainBundle::Valid::None
            ? bundle->get(&Toolchain::compilerCommand).toVariant()
            : QVariant();
    }
    return {};
}

struct ToolchainTreeItem
{
    ToolchainTreeItem() = default;
    explicit ToolchainTreeItem(const ToolchainBundle &b)
        : bundle(b) {}

    friend bool operator==(const ToolchainTreeItem &a, const ToolchainTreeItem &b)
    {
        return a.bundle == b.bundle;
    }

    std::optional<ToolchainBundle> bundle;
};

class DetectionSettingsDialog : public QDialog
{
public:
    DetectionSettingsDialog(const ToolchainDetectionSettings &settings, QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle(Tr::tr("Toolchain Auto-detection Settings"));
        const auto layout = new QVBoxLayout(this);
        m_detectX64AsX32CheckBox.setText(Tr::tr("Detect x86_64 GCC compilers "
                                                "as x86_64 and x86"));
        m_detectX64AsX32CheckBox.setToolTip(
            Tr::tr("If checked, %1 will "
                   "set up two instances of each x86_64 compiler:\nOne for the native x86_64 "
                   "target, and "
                   "one for a plain x86 target.\nEnable this if you plan to create 32-bit x86 "
                   "binaries "
                   "without using a dedicated cross compiler.")
                .arg(QGuiApplication::applicationDisplayName()));
        m_detectX64AsX32CheckBox.setChecked(settings.detectX64AsX32);
        layout->addWidget(&m_detectX64AsX32CheckBox);
        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttonBox);
    }

    ToolchainDetectionSettings settings() const
    {
        ToolchainDetectionSettings s;
        s.detectX64AsX32 = m_detectX64AsX32CheckBox.isChecked();
        return s;
    }

private:
    QCheckBox m_detectX64AsX32CheckBox;
};

// --------------------------------------------------------------------------
// ToolChainOptionsWidget
// --------------------------------------------------------------------------


#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)

class StackedWidget final : public QStackedWidget
{
    Q_OBJECT

public:
    using QStackedWidget::QStackedWidget;

    bool event(QEvent *ev) final
    {
        const bool res = QStackedWidget::event(ev);

        if (ev->type() == QEvent::ChildAdded) {
            QChildEvent *childEvent = static_cast<QChildEvent *>(ev);
            if (QWidget *w = qobject_cast<QWidget *>(childEvent->child())) {
                QTimer::singleShot(0, this, [this, w = QPointer(w)] {
                    QTC_ASSERT(w, return);
                    const int idx = indexOf(w.get());
                    emit widgetAdded(idx);
                });
            }
        }

        return res;
    }

signals:
    void widgetAdded(int index);
};

#else

using StackedWidget = QStackedWidget;

#endif

} // namespace ProjectExplorer::Internal

Q_DECLARE_METATYPE(ProjectExplorer::Internal::ToolchainTreeItem)

namespace ProjectExplorer::Internal {

class ToolchainModel final : public TypedGroupedModel<ToolchainTreeItem>
{
public:
    explicit ToolchainModel(StackedWidget *widgetStack);
    ~ToolchainModel();

    int insertBundle(const ToolchainBundle &bundle, bool changed = false);
    int addBundle(const ToolchainBundle &bundle);
    ToolchainConfigWidget *widget(int row);
    int rowForBundleId(const Id &id) const;
    void markForRemoval(int row);
    void destroyBundle(int row);
    void apply() override;

    Guard m_registerGuard;
    Guard m_deregisterGuard;

private:
    QVariant variantData(const QVariant &v, int column, int role) const override;

    StackedWidget * const m_widgetStack;
    QMap<Id, ToolchainConfigWidget *> m_widgets;
};

ToolchainModel::ToolchainModel(StackedWidget *widgetStack)
    : m_widgetStack(widgetStack)
{
    setHeader({Tr::tr("Name"), Tr::tr("Type"), Tr::tr("Language")});
    setFilters(Constants::msgAutoDetected(), {{Constants::msgManual(), [this](int row) {
        const ToolchainTreeItem it = item(row);
        return it.bundle && !it.bundle->detectionSource().isAutoDetected();
    }}});
}

ToolchainModel::~ToolchainModel()
{
    for (int row = 0; row < itemCount(); ++row) {
        if (isAdded(row)) {
            ToolchainTreeItem it = item(row);
            if (it.bundle)
                it.bundle->deleteToolchains();
        }
    }
    qDeleteAll(m_widgets);
}

int ToolchainModel::insertBundle(const ToolchainBundle &bundle, bool changed)
{
    const int row = appendItem(ToolchainTreeItem{bundle});
    if (changed)
        setChanged(row, true);
    return row;
}

int ToolchainModel::addBundle(const ToolchainBundle &bundle)
{
    return appendVolatileItem(ToolchainTreeItem{bundle});
}

ToolchainConfigWidget *ToolchainModel::widget(int row)
{
    const ToolchainTreeItem it = item(row);
    if (!it.bundle || !it.bundle->factory())
        return nullptr;
    const Id bundleId = it.bundle->bundleId();
    ToolchainConfigWidget *&w = m_widgets[bundleId];
    if (!w) {
        w = it.bundle->factory()->createConfigurationWidget(*it.bundle).release();
        if (w) {
            m_widgetStack->addWidget(w);
            if (it.bundle->detectionSource().isAutoDetected())
                w->makeReadOnly();
            connect(w, &ToolchainConfigWidget::dirty, this, [this, bundleId] {
                const int r = rowForBundleId(bundleId);
                if (r < 0)
                    return;
                setChanged(r, true);
                notifyRowChanged(r);
            });
        }
    }
    return w;
}

int ToolchainModel::rowForBundleId(const Id &id) const
{
    for (int row = 0; row < itemCount(); ++row) {
        const ToolchainTreeItem it = item(row);
        if (it.bundle && it.bundle->bundleId() == id)
            return row;
    }
    return -1;
}

void ToolchainModel::markForRemoval(int row)
{
    if (isAdded(row)) {
        ToolchainTreeItem it = item(row);
        if (it.bundle) {
            delete m_widgets.take(it.bundle->bundleId());
            it.bundle->deleteToolchains();
        }
    }
    markRemoved(row);
}

void ToolchainModel::destroyBundle(int row)
{
    const ToolchainTreeItem it = item(row);
    if (it.bundle)
        delete m_widgets.take(it.bundle->bundleId());
    removeItem(row);
}

QVariant ToolchainModel::variantData(const QVariant &v, int column, int role) const
{
    if (role == Qt::FontRole)
        return {};
    return toolchainBundleData(fromVariant(v).bundle, column, role);
}

void ToolchainModel::apply()
{
    // Apply widget changes for non-removed, non-auto items.
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        const ToolchainTreeItem it = item(row);
        if (!it.bundle || it.bundle->detectionSource().isAutoDetected() || !isDirty(row))
            continue;
        if (ToolchainConfigWidget *w = m_widgets.value(it.bundle->bundleId()))
            w->apply();
    }

    // Collect widgets for removed items BEFORE deregistering: after deregisterToolchains
    // calls qDeleteAll on toolchain objects, bundleId() would be a use-after-free.
    QList<ToolchainConfigWidget *> widgetsToDelete;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row)) {
            const ToolchainTreeItem it = item(row);
            if (it.bundle)
                widgetsToDelete << m_widgets.take(it.bundle->bundleId());
        }
    }

    // Deregister removed items.
    {
        GuardLocker locker(m_deregisterGuard);
        for (int row = 0; row < itemCount(); ++row) {
            if (!isRemoved(row))
                continue;
            const ToolchainTreeItem it = item(row);
            if (it.bundle)
                ToolchainManager::deregisterToolchains(it.bundle->toolchains());
        }
    }

    // Register added toolchains. The manager takes ownership of registered pointers.
    // Rejected duplicates are removed from the model bundle so they can be freed safely.
    QStringList removedTcs;
    Toolchains notRegisteredTcs;
    {
        GuardLocker locker(m_registerGuard);
        for (int row = 0; row < itemCount(); ++row) {
            if (!isAdded(row))
                continue;
            ToolchainTreeItem it = item(row);
            if (!it.bundle)
                continue;
            const Toolchains notRegistered = ToolchainManager::registerToolchains(it.bundle->toolchains());
            if (!notRegistered.isEmpty()) {
                removedTcs << Utils::transform(notRegistered, &Toolchain::displayName);
                for (Toolchain *tc : notRegistered)
                    it.bundle->removeToolchain(tc);
                setVolatileItem(row, it);
                notRegisteredTcs << notRegistered;
            }
        }
    }
    qDeleteAll(notRegisteredTcs);

    // GroupedModel::apply() commits all non-removed volatile items (including the
    // now-registered added rows), so no explicit removal of added rows is needed.

    qDeleteAll(widgetsToDelete);

    GroupedModel::apply();

    // Show duplicate toolchain dialog.
    if (removedTcs.count() == 1) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Duplicate Compilers Detected"),
                             Tr::tr("The following compiler was already configured:<br>"
                                    "&nbsp;%1<br>"
                                    "It was not configured again.")
                                 .arg(removedTcs.at(0)));
    } else if (!removedTcs.isEmpty()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Duplicate Compilers Detected"),
                             Tr::tr("The following compilers were already configured:<br>"
                                    "&nbsp;%1<br>"
                                    "They were not configured again.")
                                 .arg(removedTcs.join(QLatin1String(",<br>&nbsp;"))));
    }
}

class ToolChainOptionsWidget final : public Core::IOptionsPageWidget
{
public:
    ToolChainOptionsWidget();

    void toolChainSelectionChanged();
    void updateState();
    void createToolchains(ToolchainFactory *factory, const QList<Id> &languages);
    void cloneToolchains();

    void handleToolchainsRegistered(const Toolchains &toolchains);
    void handleToolchainsDeregistered(const Toolchains &toolchains);

    QAction *createAction(const QString &name, ToolchainFactory *factory, const QList<Id> &languages)
    {
        auto action = new QAction(name, this);
        connect(action, &QAction::triggered, this,
                [this, factory, languages] { createToolchains(factory, languages); });
        return action;
    }

    void redetectToolchains();

    void apply() final;

private:

    StackedWidget * const m_widgetStack = new StackedWidget;
    ToolchainModel m_model{m_widgetStack};
    GroupedView m_groupedView{m_model};
    QList<ToolchainFactory *> m_factories;
    DeviceComboBox m_deviceComboBox;
    DetailsWidget *m_container;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    QPushButton *m_removeAllButton;
    QPushButton *m_redetectButton;
    QPushButton *m_detectionSettingsButton;

    ToolchainDetectionSettings m_detectionSettings;
};

ToolChainOptionsWidget::ToolChainOptionsWidget()
{
    m_detectionSettings = ToolchainManager::detectionSettings();
    m_factories = Utils::filtered(ToolchainFactory::allToolchainFactories(),
                [](ToolchainFactory *factory) { return factory->canCreate();});

    m_groupedView.view().setSortingEnabled(true);
    m_groupedView.view().sortByColumn(0, Qt::AscendingOrder);
    m_addButton = new QPushButton(Tr::tr("Add"), this);
    auto addMenu = new QMenu(this);
    for (ToolchainFactory *factory : std::as_const(m_factories)) {
        QList<Utils::Id> languages = factory->supportedLanguages();
        if (languages.isEmpty())
            continue;

        addMenu->addAction(createAction(factory->displayName(), factory, languages));
    }
    m_addButton->setMenu(addMenu);
    if (HostOsInfo::isMacHost())
        m_addButton->setStyleSheet("text-align:center;");

    m_cloneButton = new QPushButton(Tr::tr("Clone"), this);
    connect(m_cloneButton, &QAbstractButton::clicked, this, [this] { cloneToolchains(); });

    m_delButton = new QPushButton(Tr::tr("Remove"), this);

    m_removeAllButton = new QPushButton(Tr::tr("Remove All"), this);
    connect(m_removeAllButton, &QAbstractButton::clicked, this, [this] {
        bool anyRemoved = false;
        for (int row = m_model.itemCount() - 1; row >= 0; --row) {
            if (!m_model.mapFromSource(m_model.index(row, 0)).isValid())
                continue;
            const ToolchainTreeItem it = m_model.item(row);
            if (it.bundle && !it.bundle->detectionSource().isSdkProvided()) {
                m_model.markForRemoval(row);
                anyRemoved = true;
            }
        }
        if (anyRemoved)
            markSettingsDirty();
    });

    m_redetectButton = new QPushButton(Tr::tr("Re-detect"), this);
    connect(m_redetectButton, &QAbstractButton::clicked,
            this, &ToolChainOptionsWidget::redetectToolchains);

    m_detectionSettingsButton = new QPushButton(Tr::tr("Auto-detection Settings..."), this);
    connect(m_detectionSettingsButton, &QAbstractButton::clicked, this, [this] {
        DetectionSettingsDialog dlg(m_detectionSettings, this);
        if (dlg.exec() == QDialog::Accepted) {
            bool old = m_detectionSettings.detectX64AsX32;
            m_detectionSettings = dlg.settings();
            if (m_detectionSettings.detectX64AsX32 != old)
                markSettingsDirty();
        }
    });

    m_container = new DetailsWidget(this);
    m_container->setState(DetailsWidget::NoSummary);
    m_container->setVisible(false);

    m_container->setWidget(m_widgetStack);
    connect(m_widgetStack, &StackedWidget::widgetAdded, this, [this](int index) {
        installMarkSettingsDirtyTriggerRecursively(m_widgetStack->widget(index));
    });

    const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(
        ToolchainBundle::HandleMissing::CreateAndRegister);
    for (const ToolchainBundle &b : bundles)
        m_model.insertBundle(b);

    auto buttonLayout = new QVBoxLayout;
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addWidget(m_removeAllButton);
    buttonLayout->addWidget(m_redetectButton);
    buttonLayout->addWidget(m_detectionSettingsButton);
    buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    const auto deviceLayout = new QHBoxLayout;
    deviceLayout->addWidget(new QLabel("Device:"));
    deviceLayout->addWidget(&m_deviceComboBox);
    deviceLayout->addStretch(1);

    auto toolchainsLayout = new QVBoxLayout;
    toolchainsLayout->addWidget(&m_groupedView.view());
    toolchainsLayout->addWidget(m_container);

    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->addLayout(toolchainsLayout);
    horizontalLayout->addLayout(buttonLayout);

    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(deviceLayout);
    mainLayout->addLayout(horizontalLayout);

    connect(ToolchainManager::instance(), &ToolchainManager::toolchainsRegistered,
            this, &ToolChainOptionsWidget::handleToolchainsRegistered);
    connect(ToolchainManager::instance(), &ToolchainManager::toolchainsDeregistered,
            this, &ToolChainOptionsWidget::handleToolchainsDeregistered);

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, &ToolChainOptionsWidget::toolChainSelectionChanged);
    connect(ToolchainManager::instance(), &ToolchainManager::toolchainsChanged,
            this, &ToolChainOptionsWidget::toolChainSelectionChanged);

    connect(m_delButton, &QAbstractButton::clicked, this, [this] {
        const int row = m_groupedView.currentRow();
        if (row >= 0) {
            m_model.markForRemoval(row);
            markSettingsDirty();
        }
    });

    const auto updateDevice = [this](const FilePath &deviceRoot) {
        m_model.setExtraFilter(deviceRoot.isEmpty()
            ? GroupedModel::Filter{}
            : GroupedModel::Filter{[this, deviceRoot](int row) {
                  const ToolchainTreeItem it = m_model.item(row);
                  if (!it.bundle)
                      return true;
                  const FilePath path = it.bundle->get(&Toolchain::compilerCommand);
                  return path.isEmpty() || path.isSameDevice(deviceRoot);
              }});
    };
    m_deviceComboBox.setCurrentIndex(m_deviceComboBox.indexForId({}));
    m_deviceComboBox.setOnDeviceChanged(updateDevice);

    updateState();

    installMarkSettingsDirtyTriggerRecursively(this);
}

void ToolChainOptionsWidget::handleToolchainsRegistered(const Toolchains &toolchains)
{
    if (m_model.m_registerGuard.isLocked())
        return;
    GuardLocker locker(m_model.m_registerGuard);

    // Check if bundle is already in the model (e.g. one of our pending adds).
    if (!toolchains.isEmpty() && m_model.rowForBundleId(toolchains.first()->bundleId()) >= 0)
        return;

    // External registration: add new bundles.
    const QList<ToolchainBundle> bundles = ToolchainBundle::collectBundles(
        toolchains, ToolchainBundle::HandleMissing::CreateAndRegister);
    for (const ToolchainBundle &bundle : bundles)
        m_model.insertBundle(bundle);
    updateState();
}

void ToolChainOptionsWidget::handleToolchainsDeregistered(const Toolchains &toolchains)
{
    if (m_model.m_deregisterGuard.isLocked())
        return;
    GuardLocker locker(m_model.m_deregisterGuard);

    // Find affected rows (one per bundle).
    QList<int> affectedRows;
    for (Toolchain * const tc : toolchains) {
        const int row = m_model.rowForBundleId(tc->bundleId());
        if (row >= 0 && !affectedRows.contains(row))
            affectedRows << row;
    }

    // Process in reverse order to preserve row indices during removal.
    std::sort(affectedRows.begin(), affectedRows.end(), std::greater<int>());
    for (const int row : std::as_const(affectedRows)) {
        const ToolchainTreeItem it = m_model.item(row);
        if (it.bundle) {
            // Deregister remaining toolchains in the bundle (guard prevents re-entry).
            const Toolchains remaining = Utils::filtered(
                it.bundle->toolchains(), [&toolchains](Toolchain *tc) {
                    return !toolchains.contains(tc);
                });
            ToolchainManager::deregisterToolchains(remaining);
        }
        m_model.destroyBundle(row);
    }

    updateState();
}

void ToolChainOptionsWidget::redetectToolchains()
{
    // The second element is the set of toolchains for the respective bundle that were re-discovered.
    using ItemToCheck = std::pair<int, Toolchains>;
    QList<ItemToCheck> itemsToRemove;

    Toolchains knownTcs;

    // Step 1: All previously system-detected items are candidates for removal.
    for (int row = 0; row < m_model.itemCount(); ++row) {
        if (!m_model.mapFromSource(m_model.index(row, 0)).isValid())
            continue;
        const ToolchainTreeItem it = m_model.item(row);
        if (!it.bundle)
            continue;
        if (it.bundle->detectionSource().isSystemDetected())
            itemsToRemove << std::make_pair(row, Toolchains());
        else
            knownTcs << it.bundle->toolchains();
    }

    Toolchains toAdd;
    ToolchainManager::resetBadToolchains();

    // Step 2: Re-detect toolchains.
    for (const IDeviceConstPtr &device : m_deviceComboBox.selectedDevices()) {
        for (ToolchainFactory *f : ToolchainFactory::allToolchainFactories()) {
            const ToolchainDetector detector(knownTcs, device, device->toolSearchPaths());
            for (Toolchain * const tc : f->autoDetect(detector)) {
                if (knownTcs.contains(tc))
                    continue;
                knownTcs << tc;
                const auto matchItem = [&](const ItemToCheck &item) {
                    const ToolchainTreeItem it = m_model.item(item.first);
                    return it.bundle && Utils::contains(it.bundle->toolchains(),
                                                        [&](Toolchain *btc) {
                                                            return *btc == *tc;
                                                        });
                };
                if (const auto item
                    = std::find_if(itemsToRemove.begin(), itemsToRemove.end(), matchItem);
                    item != itemsToRemove.end()) {
                    item->second << tc;
                    continue;
                }
                toAdd << tc;
            }
        }
    }

    // Step 3: Items whose toolchains were all re-discovered are no longer candidates for removal.
    //    Instead, delete the re-discovered toolchains.
    //    Conversely, if not all toolchains of the bundle were re-discovered, we remove the existing
    //    item and the newly discovered toolchains are marked for re-bundling.
    for (const auto &[row, newToolchains] : std::as_const(itemsToRemove)) {
        const ToolchainTreeItem it = m_model.item(row);
        if (it.bundle && it.bundle->toolchains().size() == newToolchains.size()) {
            qDeleteAll(newToolchains);
        } else {
            toAdd << newToolchains;
            m_model.markForRemoval(row);
        }
    }

    // Step 4: Create new bundles and add items for them.
    const QList<ToolchainBundle> newBundles
        = ToolchainBundle::collectBundles(toAdd, ToolchainBundle::HandleMissing::CreateOnly);
    for (const ToolchainBundle &bundle : newBundles)
        m_model.addBundle(bundle);

    if (!itemsToRemove.isEmpty() || !toAdd.isEmpty())
        markSettingsDirty();
}

void ToolChainOptionsWidget::toolChainSelectionChanged()
{
    const int row = m_groupedView.currentRow();
    ToolchainConfigWidget *configWidget = nullptr;
    if (row >= 0 && !m_model.isRemoved(row))
        configWidget = m_model.widget(row);
    if (configWidget) {
        m_widgetStack->setCurrentWidget(configWidget);
        if (const IDeviceConstPtr dev = m_deviceComboBox.currentDevice())
            configWidget->setFallbackBrowsePath(dev->rootPath());
    }
    m_container->setVisible(configWidget != nullptr);
    updateState();
}

void ToolChainOptionsWidget::apply()
{
    // Save current selection to restore after model reset.
    const int savedRow = m_groupedView.currentRow();
    const Id savedBundleId = savedRow >= 0 && m_model.item(savedRow).bundle
        ? m_model.item(savedRow).bundle->bundleId()
        : Id{};

    m_model.apply();

    // Restore selection.
    if (savedBundleId.isValid()) {
        const int newRow = m_model.rowForBundleId(savedBundleId);
        if (newRow >= 0)
            m_groupedView.selectRow(newRow);
    }

    ToolchainManager::setDetectionSettings(m_detectionSettings);
}

void ToolChainOptionsWidget::createToolchains(ToolchainFactory *factory, const QList<Id> &languages)
{
    QTC_ASSERT(factory, return);
    QTC_ASSERT(factory->canCreate(), return);

    const Id bundleId = Id::generate();
    Toolchains toolchains;
    for (const Id lang : languages) {
        Toolchain *tc = factory->create();
        QTC_ASSERT(tc, return);

        tc->setDetectionSource(DetectionSource::Manual);
        tc->setLanguage(lang);
        tc->setBundleId(bundleId);
        toolchains << tc;
    }

    const ToolchainBundle bundle(toolchains, ToolchainBundle::HandleMissing::CreateOnly);
    m_groupedView.selectRow(m_model.addBundle(bundle));
    markSettingsDirty();
}

void ToolChainOptionsWidget::cloneToolchains()
{
    const int row = m_groupedView.currentRow();
    if (row < 0)
        return;
    const ToolchainTreeItem it = m_model.item(row);
    if (!it.bundle)
        return;

    ToolchainBundle bundle = it.bundle->clone();
    bundle.setDetectionSource(DetectionSource::Manual);
    bundle.setDisplayName(Tr::tr("Clone of %1").arg(it.bundle->displayName()));

    m_groupedView.selectRow(m_model.addBundle(bundle));
    markSettingsDirty();
}

void ToolChainOptionsWidget::updateState()
{
    bool canCopy = false;
    bool canDelete = false;
    const int row = m_groupedView.currentRow();
    if (row >= 0 && !m_model.isRemoved(row)) {
        const ToolchainTreeItem it = m_model.item(row);
        canCopy = it.bundle && it.bundle->validity() != ToolchainBundle::Valid::None;
        canDelete = it.bundle && !it.bundle->detectionSource().isSdkProvided();
    }

    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

// --------------------------------------------------------------------------
// ToolChainOptionsPage
// --------------------------------------------------------------------------

ToolChainOptionsPage::ToolChainOptionsPage()
{
    setId(Constants::TOOLCHAIN_SETTINGS_PAGE_ID);
    setDisplayName(Tr::tr("Compilers"));
    setCategory(Constants::KITS_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new ToolChainOptionsWidget; });
}

} // namespace ProjectExplorer::Internal


#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
#include "toolchainoptionspage.moc"
#endif
