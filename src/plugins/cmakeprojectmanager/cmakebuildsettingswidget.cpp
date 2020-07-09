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

#include "cmakebuildsettingswidget.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildsystem.h"
#include "cmakeconfigitem.h"
#include "cmakekitinformation.h"
#include "configmodel.h"
#include "configmodelitemdelegate.h"

#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/buildaspects.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtbuildaspects.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QGridLayout>
#include <QPushButton>
#include <QMenu>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

static QModelIndex mapToSource(const QAbstractItemView *view, const QModelIndex &idx)
{
    if (!idx.isValid())
        return idx;

    QAbstractItemModel *model = view->model();
    QModelIndex result = idx;
    while (auto proxy = qobject_cast<const QSortFilterProxyModel *>(model)) {
        result = proxy->mapToSource(result);
        model = proxy->sourceModel();
    }
    return result;
}

// --------------------------------------------------------------------
// CMakeBuildSettingsWidget:
// --------------------------------------------------------------------

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    NamedWidget(tr("CMake")),
    m_buildConfiguration(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new Utils::CategorySortFilterModel(this)),
    m_configTextFilterModel(new Utils::CategorySortFilterModel(this))
{
    QTC_CHECK(bc);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    auto container = new Utils::DetailsWidget;
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto mainLayout = new QGridLayout(details);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setColumnStretch(1, 10);

    int row = 0;
    auto buildDirAspect = bc->buildDirectoryAspect();
    connect(buildDirAspect, &ProjectConfigurationAspect::changed, this, [this]() {
        m_configModel->flush(); // clear out config cache...;
    });
    auto initialCMakeAspect = bc->aspect<InitialCMakeArgumentsAspect>();
    auto aspectWidget = new QWidget;
    LayoutBuilder aspectWidgetBuilder(aspectWidget);
    buildDirAspect->addToLayout(aspectWidgetBuilder);
    aspectWidgetBuilder.startNewRow();
    initialCMakeAspect->addToLayout(aspectWidgetBuilder);
    mainLayout->addWidget(aspectWidget, row, 0, 1, 2);
    ++row;

    auto qmlDebugAspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
    connect(qmlDebugAspect, &QtSupport::QmlDebuggingAspect::changed,
            this, [this]() { handleQmlDebugCxxFlags(); });
    auto widget = new QWidget;
    LayoutBuilder builder(widget);
    qmlDebugAspect->addToLayout(builder);
    mainLayout->addWidget(widget, row, 0, 1, 2);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_warningMessageLabel = new Utils::InfoLabel({}, Utils::InfoLabel::Warning);
    m_warningMessageLabel->setVisible(false);
    mainLayout->addWidget(m_warningMessageLabel, row, 0, 1, -1, Qt::AlignHCenter);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_filterEdit = new Utils::FancyLineEdit;
    m_filterEdit->setPlaceholderText(tr("Filter"));
    m_filterEdit->setFiltering(true);
    mainLayout->addWidget(m_filterEdit, row, 0, 1, 2);

    ++row;
    auto tree = new Utils::TreeView;
    connect(tree, &Utils::TreeView::activated,
            tree, [tree](const QModelIndex &idx) { tree->edit(idx); });
    m_configView = tree;

    m_configView->viewport()->installEventFilter(this);

    m_configFilterModel->setSourceModel(m_configModel);
    m_configFilterModel->setFilterKeyColumn(0);
    m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
    m_configFilterModel->setFilterFixedString("0");

    m_configTextFilterModel->setSourceModel(m_configFilterModel);
    m_configTextFilterModel->setSortRole(Qt::DisplayRole);
    m_configTextFilterModel->setFilterKeyColumn(-1);

    connect(m_configTextFilterModel, &QAbstractItemModel::layoutChanged, this, [this]() {
        QModelIndex selectedIdx = m_configView->currentIndex();
        if (selectedIdx.isValid())
            m_configView->scrollTo(selectedIdx);
    });

    m_configView->setModel(m_configTextFilterModel);
    m_configView->setMinimumHeight(300);
    m_configView->setUniformRowHeights(true);
    m_configView->setSortingEnabled(true);
    m_configView->sortByColumn(0, Qt::AscendingOrder);
    auto stretcher = new Utils::HeaderViewStretcher(m_configView->header(), 0);
    m_configView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->setItemDelegate(new ConfigModelItemDelegate(m_buildConfiguration->project()->projectDirectory(),
                                                              m_configView));
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large, findWrapper);
    m_progressIndicator->attachToWidget(findWrapper);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator->show(); });

    mainLayout->addWidget(findWrapper, row, 0, 1, 2);

    auto buttonLayout = new QVBoxLayout;
    m_addButton = new QPushButton(tr("&Add"));
    m_addButton->setToolTip(tr("Add a new configuration value."));
    buttonLayout->addWidget(m_addButton);
    {
        m_addButtonMenu = new QMenu(this);
        m_addButtonMenu->addAction(tr("&Boolean"))->setData(
                    QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::BOOLEAN)));
        m_addButtonMenu->addAction(tr("&String"))->setData(
                    QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::STRING)));
        m_addButtonMenu->addAction(tr("&Directory"))->setData(
                    QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::DIRECTORY)));
        m_addButtonMenu->addAction(tr("&File"))->setData(
                    QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::FILE)));
        m_addButton->setMenu(m_addButtonMenu);
    }
    m_editButton = new QPushButton(tr("&Edit"));
    m_editButton->setToolTip(tr("Edit the current CMake configuration value."));
    buttonLayout->addWidget(m_editButton);
    m_setButton = new QPushButton(tr("&Set"));
    m_setButton->setToolTip(tr("Set a value in the CMake configuration."));
    buttonLayout->addWidget(m_setButton);
    m_unsetButton = new QPushButton(tr("&Unset"));
    m_unsetButton->setToolTip(tr("Unset a value in the CMake configuration."));
    buttonLayout->addWidget(m_unsetButton);
    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setToolTip(tr("Reset all unapplied changes."));
    m_resetButton->setEnabled(false);
    m_clearSelectionButton = new QPushButton(tr("Clear Selection"));
    m_clearSelectionButton->setToolTip(tr("Clear selection"));
    m_clearSelectionButton->setEnabled(false);
    buttonLayout->addWidget(m_clearSelectionButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_showAdvancedCheckBox = new QCheckBox(tr("Advanced"));
    buttonLayout->addWidget(m_showAdvancedCheckBox);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    mainLayout->addLayout(buttonLayout, row, 2);

    connect(m_configView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &, const QItemSelection &) {
                updateSelection();
    });

    ++row;
    m_reconfigureButton = new QPushButton(tr("Apply Configuration Changes"));
    m_reconfigureButton->setEnabled(false);
    mainLayout->addWidget(m_reconfigureButton, row, 0, 1, 3);

    updateAdvancedCheckBox();
    setError(bc->error());
    setWarning(bc->warning());

    connect(bc->buildSystem(), &BuildSystem::parsingStarted, this, [this] {
        updateButtonState();
        m_configView->setEnabled(false);
        m_showProgressTimer.start();
    });

    if (bc->buildSystem()->isParsing())
        m_showProgressTimer.start();
    else {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configView->expandAll();
    }

    connect(bc->target(), &Target::parsingFinished, this, [this, stretcher] {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configView->expandAll();
        m_configView->setEnabled(true);
        stretcher->stretch();
        updateButtonState();
        handleQmlDebugCxxFlags();
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::errorOccurred,
            this, [this]() {
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });
    connect(m_configTextFilterModel, &QAbstractItemModel::modelReset, this, [this, stretcher]() {
        m_configView->expandAll();
        stretcher->stretch();
    });

    connect(m_configModel, &QAbstractItemModel::dataChanged,
            this, &CMakeBuildSettingsWidget::updateButtonState);
    connect(m_configModel, &QAbstractItemModel::modelReset,
            this, &CMakeBuildSettingsWidget::updateButtonState);

    connect(m_showAdvancedCheckBox, &QCheckBox::stateChanged,
            this, &CMakeBuildSettingsWidget::updateAdvancedCheckBox);

    connect(m_filterEdit,
            &QLineEdit::textChanged,
            m_configTextFilterModel,
            [this](const QString &txt) {
                m_configTextFilterModel->setFilterRegularExpression(
                    QRegularExpression(QRegularExpression::escape(txt),
                                       QRegularExpression::CaseInsensitiveOption));
            });

    connect(m_resetButton, &QPushButton::clicked, m_configModel, &ConfigModel::resetAllChanges);
    connect(m_reconfigureButton,
            &QPushButton::clicked,
            m_buildConfiguration,
            &CMakeBuildConfiguration::runCMakeWithExtraArguments);
    connect(m_setButton, &QPushButton::clicked, this, [this]() {
        setVariableUnsetFlag(false);
    });
    connect(m_unsetButton, &QPushButton::clicked, this, [this]() {
        setVariableUnsetFlag(true);
    });
    connect(m_editButton, &QPushButton::clicked, this, [this]() {
        QModelIndex idx = m_configView->currentIndex();
        if (idx.column() != 1)
            idx = idx.sibling(idx.row(), 1);
        m_configView->setCurrentIndex(idx);
        m_configView->edit(idx);
    });
    connect(m_clearSelectionButton, &QPushButton::clicked, this, [this]() {
        m_configView->selectionModel()->clear();
    });
    connect(m_addButtonMenu, &QMenu::triggered, this, [this](QAction *action) {
        ConfigModel::DataItem::Type type =
                static_cast<ConfigModel::DataItem::Type>(action->data().value<int>());
        QString value = tr("<UNSET>");
        if (type == ConfigModel::DataItem::BOOLEAN)
            value = QString::fromLatin1("OFF");

        m_configModel->appendConfiguration(tr("<UNSET>"), value, type);
        const Utils::TreeItem *item = m_configModel->findNonRootItem([&value, type](Utils::TreeItem *item) {
                ConfigModel::DataItem dataItem = ConfigModel::dataItemFromIndex(item->index());
                return dataItem.key == tr("<UNSET>") && dataItem.type == type && dataItem.value == value;
        });
        QModelIndex idx = m_configModel->indexForItem(item);
        idx = m_configTextFilterModel->mapFromSource(m_configFilterModel->mapFromSource(idx));
        m_configView->setFocus();
        m_configView->scrollTo(idx);
        m_configView->setCurrentIndex(idx);
        m_configView->edit(idx);
    });

    connect(bc, &CMakeBuildConfiguration::errorOccurred, this, &CMakeBuildSettingsWidget::setError);
    connect(bc, &CMakeBuildConfiguration::warningOccurred, this, &CMakeBuildSettingsWidget::setWarning);

    updateFromKit();
    connect(m_buildConfiguration->target(), &ProjectExplorer::Target::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(m_buildConfiguration, &CMakeBuildConfiguration::enabledChanged,
            this, [this]() {
        setError(m_buildConfiguration->disabledReason());
    });

    updateSelection();
}

void CMakeBuildSettingsWidget::setError(const QString &message)
{
    m_buildConfiguration->buildDirectoryAspect()->setProblem(message);
}

void CMakeBuildSettingsWidget::setWarning(const QString &message)
{
    bool showWarning = !message.isEmpty();
    m_warningMessageLabel->setVisible(showWarning);
    m_warningMessageLabel->setText(message);
}

void CMakeBuildSettingsWidget::updateButtonState()
{
    const bool isParsing = m_buildConfiguration->buildSystem()->isParsing();
    const bool hasChanges = m_configModel->hasChanges();
    m_resetButton->setEnabled(hasChanges && !isParsing);
    m_reconfigureButton->setEnabled((hasChanges || m_configModel->hasCMakeChanges()) && !isParsing);

    // Update extra data in buildconfiguration
    const QList<ConfigModel::DataItem> changes = m_configModel->configurationForCMake();

    const CMakeConfig configChanges = Utils::transform(changes, [](const ConfigModel::DataItem &i) {
        CMakeConfigItem ni;
        ni.key = i.key.toUtf8();
        ni.value = i.value.toUtf8();
        ni.documentation = i.description.toUtf8();
        ni.isAdvanced = i.isAdvanced;
        ni.isUnset = i.isUnset;
        ni.inCMakeCache = i.inCMakeCache;
        ni.values = i.values;
        switch (i.type) {
        case CMakeProjectManager::ConfigModel::DataItem::BOOLEAN:
            ni.type = CMakeConfigItem::BOOL;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::FILE:
            ni.type = CMakeConfigItem::FILEPATH;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::DIRECTORY:
            ni.type = CMakeConfigItem::PATH;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::STRING:
            ni.type = CMakeConfigItem::STRING;
            break;
        case CMakeProjectManager::ConfigModel::DataItem::UNKNOWN:
        default:
            ni.type = CMakeConfigItem::INTERNAL;
            break;
        }
        return ni;
    });

    m_buildConfiguration->setExtraCMakeArguments(
        Utils::transform(configChanges, [](const CMakeConfigItem &i) { return i.toArgument(); }));
}

void CMakeBuildSettingsWidget::updateAdvancedCheckBox()
{
    if (m_showAdvancedCheckBox->isChecked()) {
        m_configFilterModel->setSourceModel(nullptr);
        m_configTextFilterModel->setSourceModel(m_configModel);

    } else {
        m_configTextFilterModel->setSourceModel(nullptr);
        m_configFilterModel->setSourceModel(m_configModel);
        m_configTextFilterModel->setSourceModel(m_configFilterModel);
    }
}

void CMakeBuildSettingsWidget::updateFromKit()
{
    const ProjectExplorer::Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    QHash<QString, QString> configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i.expandedValue(k));

    m_configModel->setConfigurationFromKit(configHash);
}

void CMakeBuildSettingsWidget::handleQmlDebugCxxFlags()
{
    bool changed = false;
    const auto aspect = m_buildConfiguration->aspect<QtSupport::QmlDebuggingAspect>();
    const bool enable = aspect->setting() == TriState::Enabled;

    const CMakeConfig configList = m_buildConfiguration->configurationFromCMake();
    const QByteArrayList cxxFlags{"CMAKE_CXX_FLAGS", "CMAKE_CXX_FLAGS_DEBUG",
                                  "CMAKE_CXX_FLAGS_RELWITHDEBINFO"};
    const QByteArray qmlDebug("-DQT_QML_DEBUG");

    CMakeConfig changedConfig;

    for (const CMakeConfigItem &item : configList) {
        if (!cxxFlags.contains(item.key))
            continue;

        CMakeConfigItem it(item);
        if (enable) {
            if (!it.value.contains(qmlDebug)) {
                it.value = it.value.append(' ').append(qmlDebug);
                changed = true;
            }
        } else {
            int index = it.value.indexOf(qmlDebug);
            if (index != -1) {
                it.value.remove(index, qmlDebug.length());
                changed = true;
            }
        }
        it.value = it.value.trimmed();
        changedConfig.append(it);
    }

    if (changed) {
        m_buildConfiguration->setExtraCMakeArguments(
            Utils::transform(changedConfig,
                             [](const CMakeConfigItem &i) { return i.toArgument(); }));
    }
}

void CMakeBuildSettingsWidget::updateSelection()
{
    const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();
    unsigned int setableCount = 0;
    unsigned int unsetableCount = 0;
    unsigned int editableCount = 0;

    for (const QModelIndex &index : selectedIndexes) {
        if (index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable)) {
            ConfigModel::DataItem di = m_configModel->dataItemFromIndex(index);
            if (di.isUnset)
                setableCount++;
            else
                unsetableCount++;
        }
        if (index.isValid() && index.flags().testFlag(Qt::ItemIsEditable))
            editableCount++;
    }

    m_clearSelectionButton->setEnabled(!selectedIndexes.isEmpty());
    m_setButton->setEnabled(setableCount > 0);
    m_unsetButton->setEnabled(unsetableCount > 0);
    m_editButton->setEnabled(editableCount == 1);
}

void CMakeBuildSettingsWidget::setVariableUnsetFlag(bool unsetFlag)
{
    const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();
    bool unsetFlagToggled = false;
    for (const QModelIndex &index : selectedIndexes) {
        if (index.isValid()) {
            ConfigModel::DataItem di = m_configModel->dataItemFromIndex(index);
            if (di.isUnset != unsetFlag) {
                m_configModel->toggleUnsetFlag(mapToSource(m_configView, index));
                unsetFlagToggled = true;
            }
        }
    }

    if (unsetFlagToggled)
        updateSelection();
}

QAction *CMakeBuildSettingsWidget::createForceAction(int type, const QModelIndex &idx)
{
    auto t = static_cast<ConfigModel::DataItem::Type>(type);
    QString typeString;
    switch (type) {
    case ConfigModel::DataItem::BOOLEAN:
        typeString = tr("bool", "display string for cmake type BOOLEAN");
        break;
    case ConfigModel::DataItem::FILE:
        typeString = tr("file", "display string for cmake type FILE");
        break;
    case ConfigModel::DataItem::DIRECTORY:
        typeString = tr("directory", "display string for cmake type DIRECTORY");
        break;
    case ConfigModel::DataItem::STRING:
        typeString = tr("string", "display string for cmake type STRING");
        break;
    case ConfigModel::DataItem::UNKNOWN:
        return nullptr;
    }
    QAction *forceAction = new QAction(tr("Force to %1").arg(typeString), nullptr);
    forceAction->setEnabled(m_configModel->canForceTo(idx, t));
    connect(forceAction, &QAction::triggered,
            this, [this, idx, t]() { m_configModel->forceTo(idx, t); });
    return forceAction;
}

bool CMakeBuildSettingsWidget::eventFilter(QObject *target, QEvent *event)
{
    // handle context menu events:
    if (target != m_configView->viewport() || event->type() != QEvent::ContextMenu)
        return false;

    auto e = static_cast<QContextMenuEvent *>(event);
    const QModelIndex idx = mapToSource(m_configView, m_configView->indexAt(e->pos()));
    if (!idx.isValid())
        return false;

    auto menu = new QMenu(this);
    connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

    QAction *action = nullptr;
    if ((action = createForceAction(ConfigModel::DataItem::BOOLEAN, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::FILE, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::DIRECTORY, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::STRING, idx)))
        menu->addAction(action);

    menu->move(e->globalPos());
    menu->show();

    return true;
}

} // namespace Internal
} // namespace CMakeProjectManager
