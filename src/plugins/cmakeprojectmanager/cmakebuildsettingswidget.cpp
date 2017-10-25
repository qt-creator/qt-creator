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

#include "configmodel.h"
#include "configmodelitemdelegate.h"
#include "cmakekitinformation.h"
#include "cmakeproject.h"
#include "cmakebuildconfiguration.h"

#include <coreplugin/icore.h>
#include <coreplugin/find/itemviewfind.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/asconst.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/detailswidget.h>
#include <utils/fancylineedit.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/pathchooser.h>
#include <utils/progressindicator.h>
#include <utils/utilsicons.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QStyledItemDelegate>
#include <QMenu>

namespace CMakeProjectManager {
namespace Internal {

static QModelIndex mapToSource(const QAbstractItemView *view, const QModelIndex &idx)
{
    if (!idx.isValid())
        return idx;

    QAbstractItemModel *model = view->model();
    QModelIndex result = idx;
    while (QSortFilterProxyModel *proxy = qobject_cast<QSortFilterProxyModel *>(model)) {
        result = proxy->mapToSource(result);
        model = proxy->sourceModel();
    }
    return result;
}

// --------------------------------------------------------------------
// CMakeBuildSettingsWidget:
// --------------------------------------------------------------------

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new Utils::CategorySortFilterModel),
    m_configTextFilterModel(new Utils::CategorySortFilterModel)
{
    QTC_CHECK(bc);

    setDisplayName(tr("CMake"));

    auto vbox = new QVBoxLayout(this);
    vbox->setMargin(0);
    auto container = new Utils::DetailsWidget;
    container->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto mainLayout = new QGridLayout(details);
    mainLayout->setMargin(0);
    mainLayout->setColumnStretch(1, 10);

    auto project = static_cast<CMakeProject *>(bc->project());

    auto buildDirChooser = new Utils::PathChooser;
    buildDirChooser->setBaseFileName(project->projectDirectory());
    buildDirChooser->setFileName(bc->buildDirectory());
    connect(buildDirChooser, &Utils::PathChooser::rawPathChanged, this,
            [this](const QString &path) {
                m_configModel->flush(); // clear out config cache...
                m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(path));
            });

    int row = 0;
    mainLayout->addWidget(new QLabel(tr("Build directory:")), row, 0);
    mainLayout->addWidget(buildDirChooser->lineEdit(), row, 1);
    mainLayout->addWidget(buildDirChooser->buttonAtIndex(0), row, 2);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_errorLabel = new QLabel;
    m_errorLabel->setPixmap(Utils::Icons::CRITICAL.pixmap());
    m_errorLabel->setVisible(false);
    m_errorMessageLabel = new QLabel;
    m_errorMessageLabel->setVisible(false);
    auto boxLayout = new QHBoxLayout;
    boxLayout->addWidget(m_errorLabel);
    boxLayout->addWidget(m_errorMessageLabel);
    mainLayout->addLayout(boxLayout, row, 0, 1, 3, Qt::AlignHCenter);

    ++row;
    m_warningLabel = new QLabel;
    m_warningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_warningLabel->setVisible(false);
    m_warningMessageLabel = new QLabel;
    m_warningMessageLabel->setVisible(false);
    auto boxLayout2 = new QHBoxLayout;
    boxLayout2->addWidget(m_warningLabel);
    boxLayout2->addWidget(m_warningMessageLabel);
    mainLayout->addLayout(boxLayout2, row, 0, 1, 3, Qt::AlignHCenter);

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
    m_configTextFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    m_configView->setModel(m_configTextFilterModel);
    m_configView->setMinimumHeight(300);
    m_configView->setUniformRowHeights(true);
    m_configView->setSortingEnabled(true);
    m_configView->sortByColumn(0, Qt::AscendingOrder);
    auto stretcher = new Utils::HeaderViewStretcher(m_configView->header(), 0);
    m_configView->setSelectionMode(QAbstractItemView::SingleSelection);
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
        m_addButtonMenu = new QMenu;
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
    m_unsetButton = new QPushButton(tr("&Unset"));
    m_unsetButton->setToolTip(tr("Unset a value in the CMake configuration."));
    buttonLayout->addWidget(m_unsetButton);
    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setToolTip(tr("Reset all unapplied changes."));
    m_resetButton->setEnabled(false);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_showAdvancedCheckBox = new QCheckBox(tr("Advanced"));
    buttonLayout->addWidget(m_showAdvancedCheckBox);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    mainLayout->addLayout(buttonLayout, row, 2);

    connect(m_configView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &CMakeBuildSettingsWidget::updateSelection);

    ++row;
    m_reconfigureButton = new QPushButton(tr("Apply Configuration Changes"));
    m_reconfigureButton->setEnabled(false);
    mainLayout->addWidget(m_reconfigureButton, row, 0, 1, 3);

    updateAdvancedCheckBox();
    setError(bc->error());
    setWarning(bc->warning());

    connect(project, &ProjectExplorer::Project::parsingStarted, this, [this]() {
        updateButtonState();
        m_showProgressTimer.start();
    });

    if (m_buildConfiguration->isParsing())
        m_showProgressTimer.start();
    else {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configView->expandAll();
    }

    connect(project, &ProjectExplorer::Project::parsingFinished,
            this, [this, buildDirChooser, stretcher]() {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configView->expandAll();
        stretcher->stretch();
        updateButtonState();
        buildDirChooser->triggerChanged(); // refresh valid state...
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::errorOccured,
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

    connect(m_filterEdit, &QLineEdit::textChanged,
            m_configTextFilterModel, &QSortFilterProxyModel::setFilterFixedString);

    connect(m_resetButton, &QPushButton::clicked, m_configModel, &ConfigModel::resetAllChanges);
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this]() {
        m_buildConfiguration->setConfigurationForCMake(m_configModel->configurationForCMake());
    });
    connect(m_unsetButton, &QPushButton::clicked, this, [this]() {
        m_configModel->toggleUnsetFlag(mapToSource(m_configView, m_configView->currentIndex()));
    });
    connect(m_editButton, &QPushButton::clicked, this, [this]() {
        QModelIndex idx = m_configView->currentIndex();
        if (idx.column() != 1)
            idx = idx.sibling(idx.row(), 1);
        m_configView->setCurrentIndex(idx);
        m_configView->edit(idx);
    });
    connect(m_addButtonMenu, &QMenu::triggered, this, [this](QAction *action) {
        ConfigModel::DataItem::Type type =
                static_cast<ConfigModel::DataItem::Type>(action->data().value<int>());
        QString value = tr("<UNSET>");
        if (type == ConfigModel::DataItem::BOOLEAN)
            value = QString::fromLatin1("OFF");

        m_configModel->appendConfiguration(tr("<UNSET>"), value, type);
        QModelIndex idx;
        idx = m_configView->model()->index(
                    m_configView->model()->rowCount(idx) - 1, 0);
        m_configView->setCurrentIndex(idx);
        m_configView->edit(idx);
    });

    connect(bc, &CMakeBuildConfiguration::errorOccured, this, &CMakeBuildSettingsWidget::setError);
    connect(bc, &CMakeBuildConfiguration::warningOccured, this, &CMakeBuildSettingsWidget::setWarning);

    updateFromKit();
    connect(m_buildConfiguration->target(), &ProjectExplorer::Target::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(m_buildConfiguration, &CMakeBuildConfiguration::enabledChanged,
            this, [this]() {
        setError(m_buildConfiguration->disabledReason());
        setConfigurationForCMake();
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::configurationForCMakeChanged,
            this, [this]() { setConfigurationForCMake(); });

    updateSelection(QModelIndex(), QModelIndex());
}

void CMakeBuildSettingsWidget::setError(const QString &message)
{
    bool showError = !message.isEmpty();
    m_errorLabel->setVisible(showError);
    m_errorLabel->setToolTip(message);
    m_errorMessageLabel->setVisible(showError);
    m_errorMessageLabel->setText(message);
    m_errorMessageLabel->setToolTip(message);

    m_editButton->setEnabled(!showError);
    m_unsetButton->setEnabled(!showError);
    m_resetButton->setEnabled(!showError);
    m_showAdvancedCheckBox->setEnabled(!showError);
    m_filterEdit->setEnabled(!showError);
}

void CMakeBuildSettingsWidget::setWarning(const QString &message)
{
    bool showWarning = !message.isEmpty();
    m_warningLabel->setVisible(showWarning);
    m_warningLabel->setToolTip(message);
    m_warningMessageLabel->setVisible(showWarning);
    m_warningMessageLabel->setText(message);
    m_warningMessageLabel->setToolTip(message);
}

void CMakeBuildSettingsWidget::updateButtonState()
{
    const bool isParsing = m_buildConfiguration->isParsing();
    const bool hasChanges = m_configModel->hasChanges();
    m_resetButton->setEnabled(hasChanges && !isParsing);
    m_reconfigureButton->setEnabled((hasChanges || m_configModel->hasCMakeChanges()) && !isParsing);
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
    const CMakeConfig config = CMakeConfigurationKitInformation::configuration(k);

    QHash<QString, QString> configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i.expandedValue(k));

    m_configModel->setConfigurationFromKit(configHash);
}

void CMakeBuildSettingsWidget::setConfigurationForCMake()
{
    QHash<QString, QString> config;
    const CMakeConfig configList = m_buildConfiguration->configurationForCMake();
    for (const CMakeConfigItem &i : configList) {
        config.insert(QString::fromUtf8(i.key),
                      CMakeConfigItem::expandedValueOf(m_buildConfiguration->target()->kit(),
                                                       i.key, configList));
    }
    m_configModel->setConfigurationForCMake(config);
}

void CMakeBuildSettingsWidget::updateSelection(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous);

    m_editButton->setEnabled(current.isValid() && current.flags().testFlag(Qt::ItemIsEditable));
    m_unsetButton->setEnabled(current.isValid() && current.flags().testFlag(Qt::ItemIsSelectable));
}

QAction *CMakeBuildSettingsWidget::createForceAction(int type, const QModelIndex &idx)
{
    ConfigModel::DataItem::Type t = static_cast<ConfigModel::DataItem::Type>(type);
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

    QMenu *menu = new QMenu(this);
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
