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
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/pathchooser.h>
#include <utils/itemviews.h>
#include <utils/utilsicons.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpacerItem>
#include <QMenu>

namespace CMakeProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// CMakeBuildSettingsWidget:
// --------------------------------------------------------------------

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_buildConfiguration(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new QSortFilterProxyModel)
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

    auto project = static_cast<CMakeProject *>(bc->target()->project());

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
    auto tree = new Utils::TreeView;
    connect(tree, &Utils::TreeView::activated,
            tree, [tree](const QModelIndex &idx) { tree->edit(idx); });
    m_configView = tree;
    m_configFilterModel->setSourceModel(m_configModel);
    m_configFilterModel->setFilterKeyColumn(2);
    m_configFilterModel->setFilterFixedString(QLatin1String("0"));
    m_configView->setModel(m_configFilterModel);
    m_configView->setMinimumHeight(300);
    m_configView->setRootIsDecorated(false);
    m_configView->setUniformRowHeights(true);
    auto stretcher = new Utils::HeaderViewStretcher(m_configView->header(), 1);
    m_configView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->hideColumn(2); // Hide isAdvanced column
    m_configView->setItemDelegate(new ConfigModelItemDelegate(m_configView));
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Large, findWrapper);
    m_progressIndicator->attachToWidget(findWrapper);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator->show(); });

    mainLayout->addWidget(findWrapper, row, 0, 1, 2);

    auto buttonLayout = new QVBoxLayout;
    m_addButton = new QPushButton(tr("&Add"));
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
    buttonLayout->addWidget(m_editButton);
    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setEnabled(false);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_showAdvancedCheckBox = new QCheckBox(tr("Advanced"));
    buttonLayout->addWidget(m_showAdvancedCheckBox);
    buttonLayout->addItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    mainLayout->addLayout(buttonLayout, row, 2);

    ++row;
    m_reconfigureButton = new QPushButton(tr("Apply Configuration Changes"));
    m_reconfigureButton->setEnabled(false);
    mainLayout->addWidget(m_reconfigureButton, row, 0, 1, 3);

    updateAdvancedCheckBox();
    setError(bc->error());
    setWarning(bc->warning());

    connect(project, &CMakeProject::parsingStarted, this, [this]() {
        updateButtonState();
        m_showProgressTimer.start();
    });

    if (m_buildConfiguration->isParsing())
        m_showProgressTimer.start();
    else
        m_configModel->setConfiguration(m_buildConfiguration->completeCMakeConfiguration());

    connect(m_buildConfiguration, &CMakeBuildConfiguration::dataAvailable,
            this, [this, buildDirChooser, stretcher]() {
        updateButtonState();
        m_configModel->setConfiguration(m_buildConfiguration->completeCMakeConfiguration());
        stretcher->stretch();
        buildDirChooser->triggerChanged(); // refresh valid state...
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });
    connect(m_buildConfiguration, &CMakeBuildConfiguration::errorOccured,
            this, [this]() {
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
    });

    connect(m_configModel, &QAbstractItemModel::dataChanged,
            this, &CMakeBuildSettingsWidget::updateButtonState);
    connect(m_configModel, &QAbstractItemModel::modelReset,
            this, &CMakeBuildSettingsWidget::updateButtonState);

    connect(m_showAdvancedCheckBox, &QCheckBox::stateChanged,
            this, &CMakeBuildSettingsWidget::updateAdvancedCheckBox);

    connect(m_resetButton, &QPushButton::clicked, m_configModel, &ConfigModel::resetAllChanges);
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this]() {
        m_buildConfiguration->setCurrentCMakeConfiguration(m_configModel->configurationChanges());
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
            this, [this]() { setError(m_buildConfiguration->disabledReason()); });
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
    m_resetButton->setEnabled(!showError);
    m_showAdvancedCheckBox->setEnabled(!showError);
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
    // Switch between Qt::DisplayRole (everything is "0") and Qt::EditRole (advanced is "1").
    m_configFilterModel->setFilterRole(m_showAdvancedCheckBox->isChecked() ? Qt::EditRole : Qt::DisplayRole);
}

void CMakeBuildSettingsWidget::updateFromKit()
{
    const ProjectExplorer::Kit *k = m_buildConfiguration->target()->kit();
    const CMakeConfig config = CMakeConfigurationKitInformation::configuration(k);

    QHash<QString, QString> configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i.expandedValue(k));

    m_configModel->setKitConfiguration(configHash);
}

} // namespace Internal
} // namespace CMakeProjectManager
