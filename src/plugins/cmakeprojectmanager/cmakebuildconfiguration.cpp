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

#include "cmakebuildconfiguration.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeconfigitem.h"
#include "cmakekitinformation.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "configmodel.h"
#include "configmodelitemdelegate.h"

#include <android/androidconstants.h>
#include <ios/iosconstants.h>

#include <coreplugin/find/itemviewfind.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmacroexpander.h>
#include <projectexplorer/target.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QGridLayout>
#include <QLoggingCategory>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;
using namespace CMakeProjectManager::Internal;

namespace CMakeProjectManager {

static Q_LOGGING_CATEGORY(cmakeBuildConfigurationLog, "qtc.cmake.bc", QtWarningMsg);

const char CONFIGURATION_KEY[] = "CMake.Configuration";
const char DEVELOPMENT_TEAM_FLAG[] = "Ios:DevelopmentTeam:Flag";
const char PROVISIONING_PROFILE_FLAG[] = "Ios:ProvisioningProfile:Flag";

namespace Internal {

class CMakeBuildSettingsWidget : public NamedWidget
{
public:
    CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc);

    void setError(const QString &message);
    void setWarning(const QString &message);

private:
    void updateButtonState();
    void updateAdvancedCheckBox();
    void updateFromKit();
    CMakeProjectManager::CMakeConfig getQmlDebugCxxFlags();
    CMakeProjectManager::CMakeConfig getSigningFlagsChanges();

    void updateSelection();
    void setVariableUnsetFlag(bool unsetFlag);
    QAction *createForceAction(int type, const QModelIndex &idx);

    bool eventFilter(QObject *target, QEvent *event) override;

    void batchEditConfiguration();

    CMakeBuildConfiguration *m_buildConfiguration;
    QTreeView *m_configView;
    ConfigModel *m_configModel;
    CategorySortFilterModel *m_configFilterModel;
    CategorySortFilterModel *m_configTextFilterModel;
    ProgressIndicator *m_progressIndicator;
    QPushButton *m_addButton;
    QMenu *m_addButtonMenu;
    QPushButton *m_editButton;
    QPushButton *m_setButton;
    QPushButton *m_unsetButton;
    QPushButton *m_resetButton;
    QPushButton *m_clearSelectionButton;
    QCheckBox *m_showAdvancedCheckBox;
    QPushButton *m_reconfigureButton;
    QTimer m_showProgressTimer;
    FancyLineEdit *m_filterEdit;
    InfoLabel *m_warningMessageLabel;

    QPushButton *m_batchEditButton = nullptr;
};

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

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    NamedWidget(tr("CMake")),
    m_buildConfiguration(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new CategorySortFilterModel(this)),
    m_configTextFilterModel(new CategorySortFilterModel(this))
{
    QTC_CHECK(bc);

    auto vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    auto container = new DetailsWidget;
    container->setState(DetailsWidget::NoSummary);
    vbox->addWidget(container);

    auto details = new QWidget(container);
    container->setWidget(details);

    auto mainLayout = new QGridLayout(details);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setColumnStretch(1, 10);

    int row = 0;
    auto buildDirAspect = bc->buildDirectoryAspect();
    connect(buildDirAspect, &BaseAspect::changed, this, [this]() {
        m_configModel->flush(); // clear out config cache...;
    });
    auto initialCMakeAspect = bc->aspect<InitialCMakeArgumentsAspect>();
    auto aspectWidget = new QWidget;
    Layouting::Form aspectWidgetBuilder;
    buildDirAspect->addToLayout(aspectWidgetBuilder);
    aspectWidgetBuilder.finishRow();
    initialCMakeAspect->addToLayout(aspectWidgetBuilder);
    aspectWidgetBuilder.attachTo(aspectWidget, false);
    mainLayout->addWidget(aspectWidget, row, 0, 1, -1);
    ++row;

    auto qmlDebugAspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
    connect(qmlDebugAspect, &QtSupport::QmlDebuggingAspect::changed, this, [this]() {
        updateButtonState();
    });
    Layouting::Form builder;
    qmlDebugAspect->addToLayout(builder);
    auto widget = builder.emerge();
    mainLayout->addWidget(widget, row, 0, 1, -1);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_warningMessageLabel = new InfoLabel({}, InfoLabel::Warning);
    m_warningMessageLabel->setVisible(false);
    mainLayout->addWidget(m_warningMessageLabel, row, 0, 1, -1, Qt::AlignHCenter);

    ++row;
    mainLayout->addItem(new QSpacerItem(20, 10), row, 0);

    ++row;
    m_filterEdit = new FancyLineEdit;
    m_filterEdit->setPlaceholderText(tr("Filter"));
    m_filterEdit->setFiltering(true);
    mainLayout->addWidget(m_filterEdit, row, 0, 1, 2);

    ++row;
    auto tree = new TreeView;
    connect(tree, &TreeView::activated,
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
    auto stretcher = new HeaderViewStretcher(m_configView->header(), 0);
    m_configView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->setItemDelegate(new ConfigModelItemDelegate(m_buildConfiguration->project()->projectDirectory(),
                                                              m_configView));
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large, findWrapper);
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
    m_clearSelectionButton->setToolTip(tr("Clear selection."));
    m_clearSelectionButton->setEnabled(false);
    buttonLayout->addWidget(m_clearSelectionButton);
    buttonLayout->addWidget(m_resetButton);
    m_batchEditButton = new QPushButton(tr("Batch Edit..."));
    m_batchEditButton->setToolTip(tr("Set or reset multiple values in the CMake Configuration."));
    buttonLayout->addWidget(m_batchEditButton);
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

    connect(m_buildConfiguration,
            &CMakeBuildConfiguration::signingFlagsChanged,
            this,
            &CMakeBuildSettingsWidget::updateButtonState);

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
        const TreeItem *item = m_configModel->findNonRootItem([&value, type](TreeItem *item) {
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
    connect(m_batchEditButton, &QAbstractButton::clicked,
            this, &CMakeBuildSettingsWidget::batchEditConfiguration);

    connect(bc, &CMakeBuildConfiguration::errorOccurred, this, &CMakeBuildSettingsWidget::setError);
    connect(bc, &CMakeBuildConfiguration::warningOccurred, this, &CMakeBuildSettingsWidget::setWarning);

    updateFromKit();
    connect(m_buildConfiguration->target(), &Target::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(m_buildConfiguration, &CMakeBuildConfiguration::enabledChanged,
            this, [this]() {
        setError(m_buildConfiguration->disabledReason());
    });

    updateSelection();
}


void CMakeBuildSettingsWidget::batchEditConfiguration()
{
    auto dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Edit CMake Configuration"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    auto layout = new QVBoxLayout(dialog);
    auto editor = new QPlainTextEdit(dialog);

    auto label = new QLabel(dialog);
    label->setText(tr("Enter one CMake variable per line.\n"
       "To set or change a variable, use -D<variable>:<type>=<value>.\n"
       "<type> can have one of the following values: FILEPATH, PATH, BOOL, INTERNAL, or STRING.\n"
       "To unset a variable, use -U<variable>.\n"));
    editor->setMinimumSize(800, 200);

    auto chooser = new Utils::VariableChooser(dialog);
    chooser->addSupportedWidget(editor);
    chooser->addMacroExpanderProvider([this]() { return m_buildConfiguration->macroExpander(); });

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    layout->addWidget(editor);
    layout->addWidget(label);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::accepted, this, [=]{
        const auto expander = m_buildConfiguration->macroExpander();

        const QStringList lines = editor->toPlainText().split('\n', Qt::SkipEmptyParts);
        const QStringList expandedLines = Utils::transform(lines,
                                           [expander](const QString &s) {
                                               return expander->expand(s);
                                           });
        const CMakeConfig config = CMakeConfigItem::itemsFromArguments(expandedLines);

        m_configModel->setBatchEditConfiguration(config);
    });

    editor->setPlainText(m_buildConfiguration->configurationChangesArguments().join('\n'));

    dialog->show();
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

    // Update extra data in buildconfiguration
    const QList<ConfigModel::DataItem> changes = m_configModel->configurationForCMake();

    const CMakeConfig configChanges
        = getQmlDebugCxxFlags() + getSigningFlagsChanges()
          + Utils::transform(changes, [](const ConfigModel::DataItem &i) {
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
                    ni.type = CMakeConfigItem::UNINITIALIZED;
                    break;
                }
                return ni;
            });

    m_resetButton->setEnabled(m_configModel->hasChanges() && !isParsing);
    m_reconfigureButton->setEnabled(!configChanges.isEmpty() && !isParsing);
    m_buildConfiguration->setConfigurationChanges(configChanges);
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
    const Kit *k = m_buildConfiguration->kit();
    const CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    QHash<QString, QString> configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i.expandedValue(k));

    m_configModel->setConfigurationFromKit(configHash);
}

CMakeConfig CMakeBuildSettingsWidget::getQmlDebugCxxFlags()
{
    const auto aspect = m_buildConfiguration->aspect<QtSupport::QmlDebuggingAspect>();
    const TriState qmlDebuggingState = aspect->value();
    if (qmlDebuggingState == TriState::Default) // don't touch anything
        return {};
    const bool enable = aspect->value() == TriState::Enabled;

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
                it.value = it.value.append(' ').append(qmlDebug).trimmed();
                changedConfig.append(it);
            }
        } else {
            int index = it.value.indexOf(qmlDebug);
            if (index != -1) {
                it.value.remove(index, qmlDebug.length());
                it.value = it.value.trimmed();
                changedConfig.append(it);
            }
        }
    }
    return changedConfig;
}

CMakeConfig CMakeBuildSettingsWidget::getSigningFlagsChanges()
{
    const CMakeConfig flags = m_buildConfiguration->signingFlags();
    if (flags.isEmpty())
        return {};
    const CMakeConfig configList = m_buildConfiguration->configurationFromCMake();
    if (configList.isEmpty()) {
        // we don't have any configuration --> initial configuration takes care of this itself
        return {};
    }
    CMakeConfig changedConfig;
    for (const CMakeConfigItem &signingFlag : flags) {
        const CMakeConfigItem existingFlag = Utils::findOrDefault(configList,
                                                                  Utils::equal(&CMakeConfigItem::key,
                                                                               signingFlag.key));
        const bool notInConfig = existingFlag.key.isEmpty();
        if (notInConfig != signingFlag.isUnset || existingFlag.value != signingFlag.value)
            changedConfig.append(signingFlag);
    }
    return changedConfig;
}

void CMakeBuildSettingsWidget::updateSelection()
{
    const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();
    unsigned int setableCount = 0;
    unsigned int unsetableCount = 0;
    unsigned int editableCount = 0;

    for (const QModelIndex &index : selectedIndexes) {
        if (index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable)) {
            const ConfigModel::DataItem di = ConfigModel::dataItemFromIndex(index);
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
            const ConfigModel::DataItem di = ConfigModel::dataItemFromIndex(index);
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

    auto copy = new QAction(tr("Copy"), this);
    menu->addAction(copy);
    connect(copy, &QAction::triggered, this, [this] {
        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
            return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
        });

        const QStringList variableList = Utils::transform(validIndexes, [this](const QModelIndex &index) {
            return ConfigModel::dataItemFromIndex(index)
                    .toCMakeConfigItem().toArgument(m_buildConfiguration->macroExpander());
        });

        QApplication::clipboard()->setText(variableList.join('\n'), QClipboard::Clipboard);
    });

    menu->move(e->globalPos());
    menu->show();

    return true;
}

static bool isIos(const Kit *k)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

static QStringList defaultInitialCMakeArguments(const Kit *k, const QString buildType)
{
    // Generator:
    QStringList initialArgs = CMakeGeneratorKitAspect::generatorArguments(k);

    // CMAKE_BUILD_TYPE:
    if (!buildType.isEmpty() && !CMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
        initialArgs.append(QString::fromLatin1("-DCMAKE_BUILD_TYPE:String=%1").arg(buildType));
    }

    Internal::CMakeSpecificSettings *settings
        = Internal::CMakeProjectPlugin::projectTypeSpecificSettings();

    // Package manager
    if (settings->packageManagerAutoSetup())
        initialArgs.append(QString::fromLatin1("-DCMAKE_PROJECT_INCLUDE_BEFORE:PATH=%1")
                           .arg("%{IDE:ResourcePath}/package-manager/auto-setup.cmake"));

    // Cross-compilation settings:
    if (!isIos(k)) { // iOS handles this differently
        const QString sysRoot = SysRootKitAspect::sysRoot(k).toString();
        if (!sysRoot.isEmpty()) {
            initialArgs.append(QString::fromLatin1("-DCMAKE_SYSROOT:PATH=%1").arg(sysRoot));
            if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(k)) {
                const QString targetTriple = tc->originalTargetTriple();
                initialArgs.append(
                    QString::fromLatin1("-DCMAKE_C_COMPILER_TARGET:STRING=%1").arg(targetTriple));
                initialArgs.append(
                    QString::fromLatin1("-DCMAKE_CXX_COMPILER_TARGET:STRING=%1").arg(targetTriple));
            }
        }
    }

    initialArgs += CMakeConfigurationKitAspect::toArgumentsList(k);

    return initialArgs;
}

} // namespace Internal

// -----------------------------------------------------------------------------
// CMakeBuildConfiguration:
// -----------------------------------------------------------------------------

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    m_buildSystem = new CMakeBuildSystem(this);

    buildDirectoryAspect()->setFileDialogOnly(true);
    const auto buildDirAspect = aspect<BuildDirectoryAspect>();
    buildDirAspect->setFileDialogOnly(true);
    buildDirAspect->setValueAcceptor(
        [](const QString &oldDir, const QString &newDir) -> Utils::optional<QString> {
            if (oldDir.isEmpty())
                return newDir;

            if (QDir(oldDir).exists("CMakeCache.txt") && !QDir(newDir).exists("CMakeCache.txt")) {
                if (QMessageBox::information(nullptr,
                                             tr("Changing Build Directory"),
                                             tr("Change the build directory and start with a "
                                                "basic CMake configuration?"),
                                             QMessageBox::Ok,
                                             QMessageBox::Cancel)
                    == QMessageBox::Ok) {
                    return newDir;
                }
                return Utils::nullopt;
            }
            return newDir;
        });

    auto initialCMakeArgumentsAspect = addAspect<InitialCMakeArgumentsAspect>();
    initialCMakeArgumentsAspect->setMacroExpanderProvider([this]{ return macroExpander(); });
    macroExpander()->registerVariable(DEVELOPMENT_TEAM_FLAG,
                                      tr("The CMake flag for the development team"),
                                      [this] {
                                          const CMakeConfig flags = signingFlags();
                                          if (!flags.isEmpty())
                                              return flags.first().toArgument();
                                          return QString();
                                      });
    macroExpander()->registerVariable(PROVISIONING_PROFILE_FLAG,
                                      tr("The CMake flag for the provisioning profile"),
                                      [this] {
                                          const CMakeConfig flags = signingFlags();
                                          if (flags.size() > 1 && !flags.at(1).isUnset) {
                                              return flags.at(1).toArgument();
                                          }
                                          return QString();
                                      });

    addAspect<SourceDirectoryAspect>();
    addAspect<BuildTypeAspect>();

    appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *k = target->kit();

        QStringList initialArgs = defaultInitialCMakeArguments(k, info.typeName);

        // Android magic:
        if (DeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
            const auto &bs = buildSteps()->steps().constLast();
            initialArgs.append(
                QString::fromLatin1("-DANDROID_NATIVE_API_LEVEL:STRING=%1")
                    .arg(bs->data(Android::Constants::AndroidNdkPlatform).toString()));
            auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
            initialArgs.append(
                QString::fromLatin1("-DANDROID_NDK:PATH=%1").arg(ndkLocation.toString()));

            initialArgs.append(
                QString::fromLatin1("-DCMAKE_TOOLCHAIN_FILE:PATH=%1")
                    .arg(
                        ndkLocation.pathAppended("build/cmake/android.toolchain.cmake").toString()));

            auto androidAbis = bs->data(Android::Constants::AndroidABIs).toStringList();
            QString preferredAbi;
            if (androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)) {
                preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A;
            } else if (androidAbis.isEmpty()
                       || androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)) {
                preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A;
            } else {
                preferredAbi = androidAbis.first();
            }
            initialArgs.append(QString::fromLatin1("-DANDROID_ABI:STRING=%1").arg(preferredAbi));

            initialArgs.append(QString::fromLatin1("-DANDROID_STL:STRING=c++_shared"));

            initialArgs.append(
                QString::fromLatin1("-DCMAKE_FIND_ROOT_PATH:PATH=%{Qt:QT_INSTALL_PREFIX}"));

            QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();

            if (qt->qtVersion() >= QtSupport::QtVersionNumber{6, 0, 0}) {
                initialArgs.append(
                    QString::fromLatin1("-DQT_HOST_PATH:PATH=%{Qt:QT_HOST_PREFIX}"));

                initialArgs.append(QString("-DANDROID_SDK_ROOT:PATH=%1").arg(sdkLocation.toString()));
            } else {
                initialArgs.append(QString("-DANDROID_SDK:PATH=%1").arg(sdkLocation.toString()));
            }
        }

        if (isIos(k)) {
            QtSupport::BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            if (qt && qt->qtVersion().majorVersion >= 6) {
                // TODO it would be better if we could set
                // CMAKE_SYSTEM_NAME=iOS and CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES
                // and build with "cmake --build . -- -arch <arch>" instead of setting the architecture
                // and sysroot in the CMake configuration, but that currently doesn't work with Qt/CMake
                // https://gitlab.kitware.com/cmake/cmake/-/issues/21276
                const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
                // TODO the architectures are probably not correct with Apple Silicon in the mix...
                const QString architecture = deviceType == Ios::Constants::IOS_DEVICE_TYPE
                                                 ? QLatin1String("arm64")
                                                 : QLatin1String("x86_64");
                const QString sysroot = deviceType == Ios::Constants::IOS_DEVICE_TYPE
                                            ? QLatin1String("iphoneos")
                                            : QLatin1String("iphonesimulator");
                initialArgs.append("-DCMAKE_TOOLCHAIN_FILE:PATH=%{Qt:QT_INSTALL_PREFIX}/lib/cmake/"
                                   "Qt6/qt.toolchain.cmake");
                initialArgs.append("-DCMAKE_OSX_ARCHITECTURES:STRING=" + architecture);
                initialArgs.append("-DCMAKE_OSX_SYSROOT:STRING=" + sysroot);
                initialArgs.append("%{" + QLatin1String(DEVELOPMENT_TEAM_FLAG) + "}");
                initialArgs.append("%{" + QLatin1String(PROVISIONING_PROFILE_FLAG) + "}");
            }
        }

        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.displayName,
                                                   info.buildType));
        }

        if (info.extraInfo.isValid()) {
            setSourceDirectory(FilePath::fromVariant(
                        info.extraInfo.value<QVariantMap>().value(Constants::CMAKE_HOME_DIR)));
        }

        setInitialCMakeArguments(initialArgs);
        setCMakeBuildType(info.typeName);
    });

    const auto qmlDebuggingAspect = addAspect<QtSupport::QmlDebuggingAspect>();
    qmlDebuggingAspect->setKit(target->kit());
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_buildSystem;
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    const CMakeConfig conf
            = Utils::filtered(Utils::transform(map.value(QLatin1String(CONFIGURATION_KEY)).toStringList(),
                                               [](const QString &v) { return CMakeConfigItem::fromString(v); }),
                              [](const CMakeConfigItem &c) { return !c.isNull(); });

    // TODO: Upgrade from Qt Creator < 4.13: Remove when no longer supported!
    const QString buildTypeName = [this]() {
        switch (buildType()) {
        case Debug:
            return QString("Debug");
        case Profile:
            return QString("RelWithDebInfo");
        case Release:
            return QString("Release");
        case Unknown:
        default:
            return QString("");
        }
    }();
    if (initialCMakeArguments().isEmpty()) {
        QStringList initialArgs = defaultInitialCMakeArguments(kit(), buildTypeName)
                                  + Utils::transform(conf, [this](const CMakeConfigItem &i) {
                                        return i.toArgument(macroExpander());
                                    });

        setInitialCMakeArguments(initialArgs);
    }

    return true;
}

FilePath CMakeBuildConfiguration::shadowBuildDirectory(const FilePath &projectFilePath,
                                                       const Kit *k,
                                                       const QString &bcName,
                                                       BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return FilePath();

    const QString projectName = projectFilePath.parentDir().fileName();
    ProjectMacroExpander expander(projectFilePath, projectName, k, bcName, buildType);
    QDir projectDir = QDir(Project::projectDirectory(projectFilePath).toString());
    QString buildPath = expander.expand(ProjectExplorerPlugin::buildDirectoryTemplate());
    buildPath.replace(" ", "-");

    if (CMakeGeneratorKitAspect::isMultiConfigGenerator(k))
        buildPath = buildPath.left(buildPath.lastIndexOf(QString("-%1").arg(bcName)));

    return FilePath::fromUserInput(projectDir.absoluteFilePath(buildPath));
}

void CMakeBuildConfiguration::buildTarget(const QString &buildTarget)
{
    auto cmBs = qobject_cast<CMakeBuildStep *>(findOrDefault(
                                                   buildSteps()->steps(),
                                                   [](const BuildStep *bs) {
        return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
    }));

    QStringList originalBuildTargets;
    if (cmBs) {
        originalBuildTargets = cmBs->buildTargets();
        cmBs->setBuildTargets({buildTarget});
    }

    BuildManager::buildList(buildSteps());

    if (cmBs)
        cmBs->setBuildTargets(originalBuildTargets);
}

CMakeConfig CMakeBuildConfiguration::configurationFromCMake() const
{
    return m_configurationFromCMake;
}

CMakeConfig CMakeBuildConfiguration::configurationChanges() const
{
    return m_configurationChanges;
}

QStringList CMakeBuildConfiguration::configurationChangesArguments() const
{
    return Utils::transform(m_configurationChanges, [](const CMakeConfigItem &i) { return i.toArgument(); });
}

QStringList CMakeBuildConfiguration::initialCMakeArguments() const
{
    return aspect<InitialCMakeArgumentsAspect>()->value().split('\n', Qt::SkipEmptyParts);
}

void CMakeBuildConfiguration::setConfigurationFromCMake(const CMakeConfig &config)
{
    m_configurationFromCMake = config;
}

void CMakeBuildConfiguration::setConfigurationChanges(const CMakeConfig &config)
{
    qCDebug(cmakeBuildConfigurationLog)
        << "Configuration changes before:" << configurationChangesArguments();

    m_configurationChanges = config;

    qCDebug(cmakeBuildConfigurationLog)
        << "Configuration changes after:" << configurationChangesArguments();
}

// FIXME: Run clean steps when a setting starting with "ANDROID_BUILD_ABI_" is changed.
// FIXME: Warn when kit settings are overridden by a project.

void CMakeBuildConfiguration::clearError(ForceEnabledChanged fec)
{
    if (!m_error.isEmpty()) {
        m_error.clear();
        fec = ForceEnabledChanged::True;
    }
    if (fec == ForceEnabledChanged::True) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit enabledChanged();
    }
}

void CMakeBuildConfiguration::setInitialCMakeArguments(const QStringList &args)
{
    aspect<InitialCMakeArgumentsAspect>()->setValue(args.join('\n'));
}

void CMakeBuildConfiguration::setError(const QString &message)
{
    qCDebug(cmakeBuildConfigurationLog) << "Setting error to" << message;
    QTC_ASSERT(!message.isEmpty(), return );

    const QString oldMessage = m_error;
    if (m_error != message)
        m_error = message;
    if (oldMessage.isEmpty() != !message.isEmpty()) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit enabledChanged();
    }
    emit errorOccurred(m_error);
}

void CMakeBuildConfiguration::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    emit warningOccurred(m_warning);
}

QString CMakeBuildConfiguration::error() const
{
    return m_error;
}

QString CMakeBuildConfiguration::warning() const
{
    return m_warning;
}

NamedWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

CMakeConfig CMakeBuildConfiguration::signingFlags() const
{
    return {};
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<CMakeBuildConfiguration>(Constants::CMAKE_BUILDCONFIGURATION_ID);

    setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::CMAKE_PROJECT_MIMETYPE);

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        FilePath path = forSetup ? Project::projectDirectory(projectPath) : projectPath;

        for (int type = BuildTypeDebug; type != BuildTypeLast; ++type) {
            BuildInfo info = createBuildInfo(BuildType(type));
            if (forSetup) {
                info.buildDirectory = CMakeBuildConfiguration::shadowBuildDirectory(projectPath,
                                k,
                                info.typeName,
                                info.buildType);
            }
            result << info;
        }
        return result;
    });
}

CMakeBuildConfigurationFactory::BuildType CMakeBuildConfigurationFactory::buildTypeFromByteArray(
    const QByteArray &in)
{
    const QByteArray bt = in.toLower();
    if (bt == "debug")
        return BuildTypeDebug;
    if (bt == "release")
        return BuildTypeRelease;
    if (bt == "relwithdebinfo")
        return BuildTypeRelWithDebInfo;
    if (bt == "minsizerel")
        return BuildTypeMinSizeRel;
    return BuildTypeNone;
}

BuildConfiguration::BuildType CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(
    const CMakeBuildConfigurationFactory::BuildType &in)
{
    // Cover all common CMake build types
    if (in == BuildTypeRelease || in == BuildTypeMinSizeRel)
        return BuildConfiguration::Release;
    else if (in == BuildTypeDebug)
        return BuildConfiguration::Debug;
    else if (in == BuildTypeRelWithDebInfo)
        return BuildConfiguration::Profile;
    else
        return BuildConfiguration::Unknown;
}

BuildInfo CMakeBuildConfigurationFactory::createBuildInfo(BuildType buildType)
{
    BuildInfo info;

    switch (buildType) {
    case BuildTypeNone:
        info.typeName = "Build";
        info.displayName = BuildConfiguration::tr("Build");
        info.buildType = BuildConfiguration::Unknown;
        break;
    case BuildTypeDebug:
        info.typeName = "Debug";
        info.displayName = BuildConfiguration::tr("Debug");
        info.buildType = BuildConfiguration::Debug;
        break;
    case BuildTypeRelease:
        info.typeName = "Release";
        info.displayName = BuildConfiguration::tr("Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        info.typeName = "MinSizeRel";
        info.displayName = CMakeBuildConfiguration::tr("Minimum Size Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        info.typeName = "RelWithDebInfo";
        info.displayName = CMakeBuildConfiguration::tr("Release with Debug Information");
        info.buildType = BuildConfiguration::Profile;
        break;
    default:
        QTC_CHECK(false);
        break;
    }

    return info;
}

BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    QByteArray cmakeBuildTypeName = CMakeConfigItem::valueOf("CMAKE_BUILD_TYPE", m_configurationFromCMake);
    if (cmakeBuildTypeName.isEmpty()) {
        QByteArray cmakeCfgTypes = CMakeConfigItem::valueOf("CMAKE_CONFIGURATION_TYPES", m_configurationFromCMake);
        if (!cmakeCfgTypes.isEmpty())
            cmakeBuildTypeName = cmakeBuildType().toUtf8();
    }
    // Cover all common CMake build types
    const CMakeBuildConfigurationFactory::BuildType cmakeBuildType
        = CMakeBuildConfigurationFactory::buildTypeFromByteArray(cmakeBuildTypeName);
    return CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(cmakeBuildType);
}

BuildSystem *CMakeBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

void CMakeBuildConfiguration::runCMakeWithExtraArguments()
{
    m_buildSystem->runCMakeWithExtraArguments();
}

void CMakeBuildConfiguration::setSourceDirectory(const FilePath &path)
{
    aspect<SourceDirectoryAspect>()->setValue(path.toString());
}

FilePath CMakeBuildConfiguration::sourceDirectory() const
{
    return FilePath::fromString(aspect<SourceDirectoryAspect>()->value());
}

QString CMakeBuildConfiguration::cmakeBuildType() const
{
    return aspect<BuildTypeAspect>()->value();
}

void CMakeBuildConfiguration::setCMakeBuildType(const QString &cmakeBuildType)
{
    aspect<BuildTypeAspect>()->setValue(cmakeBuildType);
}

namespace Internal {

// ----------------------------------------------------------------------
// - InitialCMakeParametersAspect:
// ----------------------------------------------------------------------

InitialCMakeArgumentsAspect::InitialCMakeArgumentsAspect()
{
    setSettingsKey("CMake.Initial.Parameters");
    setLabelText(tr("Initial CMake parameters:"));
    setDisplayStyle(TextEditDisplay);
}

// -----------------------------------------------------------------------------
// SourceDirectoryAspect:
// -----------------------------------------------------------------------------
SourceDirectoryAspect::SourceDirectoryAspect()
{
    // Will not be displayed, only persisted
    setSettingsKey("CMake.Source.Directory");
}

// -----------------------------------------------------------------------------
// BuildTypeAspect:
// -----------------------------------------------------------------------------
BuildTypeAspect::BuildTypeAspect()
{
    setSettingsKey("CMake.Build.Type");
}

} // namespace Internal
} // namespace CMakeProjectManager
