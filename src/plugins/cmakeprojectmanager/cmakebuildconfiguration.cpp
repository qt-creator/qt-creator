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
#include "fileapiparser.h"

#include <android/androidconstants.h>
#include <docker/dockerconstants.h>
#include <ios/iosconstants.h>
#include <qnx/qnxconstants.h>
#include <webassembly/webassemblyconstants.h>

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/checkablemessagebox.h>
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
const char CMAKE_OSX_ARCHITECTURES_FLAG[] = "CMAKE_OSX_ARCHITECTURES:DefaultFlag";
const char CMAKE_QT6_TOOLCHAIN_FILE_ARG[] =
        "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=%{Qt:QT_INSTALL_PREFIX}/lib/cmake/Qt6/qt.toolchain.cmake";

namespace Internal {

class CMakeBuildSettingsWidget : public NamedWidget
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeBuildSettingsWidget)

public:
    CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc);

    void setError(const QString &message);
    void setWarning(const QString &message);

private:
    void updateButtonState();
    void updateAdvancedCheckBox();
    void updateFromKit();
    void updateConfigurationStateIndex(int index);
    CMakeConfig getQmlDebugCxxFlags();
    CMakeConfig getSigningFlagsChanges();

    void updateSelection();
    void updateConfigurationStateSelection();
    bool isInitialConfiguration() const;
    void setVariableUnsetFlag(bool unsetFlag);
    QAction *createForceAction(int type, const QModelIndex &idx);

    bool eventFilter(QObject *target, QEvent *event) override;

    void batchEditConfiguration();
    void reconfigureWithInitialParameters(CMakeBuildConfiguration *bc);
    void updateInitialCMakeArguments();
    void kitCMakeConfiguration();

    CMakeBuildConfiguration *m_buildConfiguration;
    QTreeView *m_configView;
    ConfigModel *m_configModel;
    CategorySortFilterModel *m_configFilterModel;
    CategorySortFilterModel *m_configTextFilterModel;
    ProgressIndicator *m_progressIndicator;
    QPushButton *m_addButton;
    QPushButton *m_editButton;
    QPushButton *m_setButton;
    QPushButton *m_unsetButton;
    QPushButton *m_resetButton;
    QCheckBox *m_showAdvancedCheckBox;
    QTabBar *m_configurationStates;
    QPushButton *m_reconfigureButton;
    QTimer m_showProgressTimer;
    FancyLineEdit *m_filterEdit;
    InfoLabel *m_warningMessageLabel;

    QPushButton *m_batchEditButton = nullptr;
    QPushButton *m_kitConfiguration = nullptr;
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

    auto buildDirAspect = bc->buildDirectoryAspect();
    buildDirAspect->setAutoApplyOnEditingFinished(true);
    connect(buildDirAspect, &BaseAspect::changed, this, [this]() {
        m_configModel->flush(); // clear out config cache...;
    });

    auto buildTypeAspect = bc->aspect<BuildTypeAspect>();
    connect(buildTypeAspect, &BaseAspect::changed, this, [this, buildTypeAspect]() {
        if (!m_buildConfiguration->isMultiConfig()) {
            CMakeConfig config;
            config << CMakeConfigItem("CMAKE_BUILD_TYPE", buildTypeAspect->value().toUtf8());

            m_configModel->setBatchEditConfiguration(config);
        }
    });

    auto qmlDebugAspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
    connect(qmlDebugAspect, &QtSupport::QmlDebuggingAspect::changed, this, [this]() {
        updateButtonState();
    });

    m_warningMessageLabel = new InfoLabel({}, InfoLabel::Warning);
    m_warningMessageLabel->setVisible(false);

    m_configurationStates = new QTabBar(this);
    m_configurationStates->addTab(tr("Initial Configuration"));
    m_configurationStates->addTab(tr("Current Configuration"));
    connect(m_configurationStates, &QTabBar::currentChanged, this, [this](int index) {
        updateConfigurationStateIndex(index);
    });

    m_kitConfiguration = new QPushButton(tr("Kit Configuration"));
    m_kitConfiguration->setToolTip(tr("Edit the current kit's CMake configuration."));
    m_kitConfiguration->setFixedWidth(m_kitConfiguration->sizeHint().width());
    connect(m_kitConfiguration, &QPushButton::clicked, this, [this]() { kitCMakeConfiguration(); });

    m_filterEdit = new FancyLineEdit;
    m_filterEdit->setPlaceholderText(tr("Filter"));
    m_filterEdit->setFiltering(true);
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
    m_configView->setAlternatingRowColors(true);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->setItemDelegate(new ConfigModelItemDelegate(m_buildConfiguration->project()->projectDirectory(),
                                                              m_configView));
    m_configView->setRootIsDecorated(false);
    QFrame *findWrapper = Core::ItemViewFind::createSearchableWrapper(m_configView, Core::ItemViewFind::LightColored);
    findWrapper->setFrameStyle(QFrame::StyledPanel);

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large, findWrapper);
    m_progressIndicator->attachToWidget(findWrapper);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { m_progressIndicator->show(); });

    m_addButton = new QPushButton(tr("&Add"));
    m_addButton->setToolTip(tr("Add a new configuration value."));
    auto addButtonMenu = new QMenu(this);
    addButtonMenu->addAction(tr("&Boolean"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::BOOLEAN)));
    addButtonMenu->addAction(tr("&String"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::STRING)));
    addButtonMenu->addAction(tr("&Directory"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::DIRECTORY)));
    addButtonMenu->addAction(tr("&File"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::FILE)));
    m_addButton->setMenu(addButtonMenu);

    m_editButton = new QPushButton(tr("&Edit"));
    m_editButton->setToolTip(tr("Edit the current CMake configuration value."));

    m_setButton = new QPushButton(tr("&Set"));
    m_setButton->setToolTip(tr("Set a value in the CMake configuration."));

    m_unsetButton = new QPushButton(tr("&Unset"));
    m_unsetButton->setToolTip(tr("Unset a value in the CMake configuration."));

    m_resetButton = new QPushButton(tr("&Reset"));
    m_resetButton->setToolTip(tr("Reset all unapplied changes."));
    m_resetButton->setEnabled(false);

    m_batchEditButton = new QPushButton(tr("Batch Edit..."));
    m_batchEditButton->setToolTip(tr("Set or reset multiple values in the CMake Configuration."));

    m_showAdvancedCheckBox = new QCheckBox(tr("Advanced"));

    connect(m_configView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &, const QItemSelection &) {
                updateSelection();
    });

    m_reconfigureButton = new QPushButton(tr("Run CMake"));
    m_reconfigureButton->setEnabled(false);

    using namespace Layouting;
    Grid cmakeConfiguration {
        m_filterEdit, Break(),
        findWrapper,
        Column {
            m_addButton,
            m_editButton,
            m_setButton,
            m_unsetButton,
            m_resetButton,
            m_batchEditButton,
            Space(10),
            m_showAdvancedCheckBox,
            Stretch()
        }
    };

    Column {
        Form {
            buildDirAspect,
            bc->aspect<BuildTypeAspect>(),
            qmlDebugAspect
        },
        m_warningMessageLabel,
        Space(10),
        Row{m_kitConfiguration, m_configurationStates},
        Group {
            cmakeConfiguration,
            Row {
                bc->aspect<InitialCMakeArgumentsAspect>(),
                bc->aspect<AdditionalCMakeOptionsAspect>()
            },
            m_reconfigureButton,
        }
    }.attachTo(details, false);

    updateAdvancedCheckBox();
    setError(bc->error());
    setWarning(bc->warning());

    connect(bc->buildSystem(), &BuildSystem::parsingStarted, this, [this] {
        updateButtonState();
        m_configView->setEnabled(false);
        m_showProgressTimer.start();
    });

    m_configModel->setMacroExpander(m_buildConfiguration->macroExpander());

    if (bc->buildSystem()->isParsing())
        m_showProgressTimer.start();
    else {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configModel->setInitialParametersConfiguration(
            m_buildConfiguration->initialCMakeConfiguration());
        m_configView->expandAll();
    }

    connect(bc->buildSystem(), &BuildSystem::parsingFinished, this, [this, stretcher] {
        m_configModel->setConfiguration(m_buildConfiguration->configurationFromCMake());
        m_configModel->setInitialParametersConfiguration(
            m_buildConfiguration->initialCMakeConfiguration());
        m_buildConfiguration->filterConfigArgumentsFromAdditionalCMakeArguments();
        updateFromKit();
        m_configView->expandAll();
        m_configView->setEnabled(true);
        stretcher->stretch();
        updateButtonState();
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
        updateConfigurationStateSelection();
    });

    auto cbc = static_cast<CMakeBuildSystem *>(bc->buildSystem());
    connect(cbc, &CMakeBuildSystem::configurationCleared, this, [this]() {
        updateConfigurationStateSelection();
    });

    connect(m_buildConfiguration, &CMakeBuildConfiguration::errorOccurred,
            this, [this]() {
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
        updateConfigurationStateSelection();
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

    connect(m_resetButton, &QPushButton::clicked, this, [this](){
        m_configModel->resetAllChanges(isInitialConfiguration());
    });
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this, bc]() {
        auto buildSystem = static_cast<CMakeBuildSystem *>(m_buildConfiguration->buildSystem());
        if (!buildSystem->isParsing()) {
            if (isInitialConfiguration()) {
                reconfigureWithInitialParameters(bc);
            } else {
                buildSystem->runCMakeWithExtraArguments();
            }
        } else {
            buildSystem->stopCMakeRun();
            m_reconfigureButton->setEnabled(false);
        }
    });
    connect(m_setButton, &QPushButton::clicked, this, [this]() { setVariableUnsetFlag(false); });
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
    connect(addButtonMenu, &QMenu::triggered, this, [this](QAction *action) {
        ConfigModel::DataItem::Type type =
                static_cast<ConfigModel::DataItem::Type>(action->data().value<int>());
        QString value = tr("<UNSET>");
        if (type == ConfigModel::DataItem::BOOLEAN)
            value = QString::fromLatin1("OFF");

        m_configModel->appendConfiguration(tr("<UNSET>"), value, type, isInitialConfiguration());
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
    connect(bc, &CMakeBuildConfiguration::configurationChanged, this, [this](const CMakeConfig &config) {
       m_configModel->setBatchEditConfiguration(config);
    });

    updateFromKit();
    connect(m_buildConfiguration->target(), &Target::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(m_buildConfiguration, &CMakeBuildConfiguration::enabledChanged,
            this, [this]() {
        if (m_buildConfiguration->isEnabled())
            setError(QString());
    });
    connect(this, &QObject::destroyed, this, [this](const QObject *obj) {
        updateInitialCMakeArguments();
    });

    connect(bc->aspect<InitialCMakeArgumentsAspect>(),
            &Utils::BaseAspect::labelLinkActivated,
            this,
            [this](const QString &link) {
                const CMakeTool *tool = CMakeKitAspect::cmakeTool(
                    m_buildConfiguration->target()->kit());
                CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake.1.html#options");
            });
    connect(bc->aspect<AdditionalCMakeOptionsAspect>(),
            &Utils::BaseAspect::labelLinkActivated,
            this,
            [this](const QString &link) {
                const CMakeTool *tool = CMakeKitAspect::cmakeTool(
                    m_buildConfiguration->target()->kit());
                CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake.1.html#options");
            });

    updateSelection();
    updateConfigurationStateSelection();
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
    label->setText(tr("Enter one CMake <a href=\"variable\">variable</a> per line.<br/>"
       "To set or change a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
       "&lt;type&gt; can have one of the following values: FILEPATH, PATH, BOOL, INTERNAL, or STRING.<br/>"
                      "To unset a variable, use -U&lt;variable&gt;.<br/>"));
    connect(label, &QLabel::linkActivated, this, [this](const QString &link) {
        const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildConfiguration->target()->kit());
        CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake-variables.7.html");
    });
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
        const bool isInitial = isInitialConfiguration();
        QStringList unknownOptions;
        CMakeConfig config = CMakeConfig::fromArguments(isInitial ? lines : expandedLines,
                                                        unknownOptions);
        for (auto &ci : config)
            ci.isInitial = isInitial;

        m_configModel->setBatchEditConfiguration(config);
    });

    editor->setPlainText(
        m_buildConfiguration->configurationChangesArguments(isInitialConfiguration())
            .join('\n'));

    dialog->show();
}

void CMakeBuildSettingsWidget::reconfigureWithInitialParameters(CMakeBuildConfiguration *bc)
{
    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    bool doNotAsk = !settings->askBeforeReConfigureInitialParams.value();
    if (!doNotAsk) {
        QDialogButtonBox::StandardButton reply = Utils::CheckableMessageBox::question(
            Core::ICore::dialogParent(),
            tr("Re-configure with Initial Parameters"),
            tr("Clear CMake configuration and configure with initial parameters?"),
            tr("Do not ask again"),
            &doNotAsk,
            QDialogButtonBox::Yes | QDialogButtonBox::No,
            QDialogButtonBox::Yes);

        settings->askBeforeReConfigureInitialParams.setValue(!doNotAsk);
        settings->writeSettings(Core::ICore::settings());

        if (reply != QDialogButtonBox::Yes) {
            return;
        }
    }

    auto cbc = static_cast<CMakeBuildSystem*>(bc->buildSystem());
    cbc->clearCMakeCache();

    updateInitialCMakeArguments();

    if (ProjectExplorerPlugin::saveModifiedFiles())
        cbc->runCMake();
}

void CMakeBuildSettingsWidget::updateInitialCMakeArguments()
{
    CMakeConfig initialList = m_buildConfiguration->initialCMakeConfiguration();

    for (const CMakeConfigItem &ci : m_buildConfiguration->configurationChanges()) {
        if (!ci.isInitial)
            continue;
        auto it = std::find_if(initialList.begin(),
                               initialList.end(),
                               [ci](const CMakeConfigItem &item) {
                                   return item.key == ci.key;
                               });
        if (it != initialList.end()) {
            *it = ci;
            if (ci.isUnset)
                initialList.erase(it);
        } else {
            initialList.push_back(ci);
        }
    }

    m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->setCMakeConfiguration(initialList);

    // value() will contain only the unknown arguments (the non -D/-U arguments)
    // As the user would expect to have e.g. "--preset" from "Initial Configuration"
    // to "Current Configuration" as additional parameters
    m_buildConfiguration->setAdditionalCMakeArguments(ProcessArgs::splitArgs(
        m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->value()));
}

void CMakeBuildSettingsWidget::kitCMakeConfiguration()
{
    m_buildConfiguration->kit()->blockNotification();

    auto dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Kit CMake Configuration"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->setSizeGripEnabled(true);
    connect(dialog, &QDialog::finished, this, [=]{
        m_buildConfiguration->kit()->unblockNotification();
    });

    CMakeKitAspect kitAspect;
    CMakeGeneratorKitAspect generatorAspect;
    CMakeConfigurationKitAspect configurationKitAspect;

    auto layout = new QGridLayout(dialog);

    kitAspect.createConfigWidget(m_buildConfiguration->kit())
        ->addToLayoutWithLabel(layout->parentWidget());
    generatorAspect.createConfigWidget(m_buildConfiguration->kit())
        ->addToLayoutWithLabel(layout->parentWidget());
    configurationKitAspect.createConfigWidget(m_buildConfiguration->kit())
        ->addToLayoutWithLabel(layout->parentWidget());

    layout->setColumnStretch(1, 1);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::clicked, dialog, &QDialog::close);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Maximum, QSizePolicy::MinimumExpanding),
                    4, 0);
    layout->addWidget(buttons, 5, 0, 1, -1);

    dialog->setMinimumWidth(400);
    dialog->resize(800, 1);
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
                ni.isInitial = i.isInitial;
                ni.isUnset = i.isUnset;
                ni.inCMakeCache = i.inCMakeCache;
                ni.values = i.values;
                switch (i.type) {
                case ConfigModel::DataItem::BOOLEAN:
                    ni.type = CMakeConfigItem::BOOL;
                    break;
                case ConfigModel::DataItem::FILE:
                    ni.type = CMakeConfigItem::FILEPATH;
                    break;
                case ConfigModel::DataItem::DIRECTORY:
                    ni.type = CMakeConfigItem::PATH;
                    break;
                case ConfigModel::DataItem::STRING:
                    ni.type = CMakeConfigItem::STRING;
                    break;
                case ConfigModel::DataItem::UNKNOWN:
                default:
                    ni.type = CMakeConfigItem::UNINITIALIZED;
                    break;
                }
                return ni;
            });

    const bool isInitial = isInitialConfiguration();
    m_resetButton->setEnabled(m_configModel->hasChanges(isInitial) && !isParsing);

    m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->setVisible(isInitialConfiguration());
    m_buildConfiguration->aspect<AdditionalCMakeOptionsAspect>()->setVisible(!isInitialConfiguration());

    m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->setEnabled(!isParsing);
    m_buildConfiguration->aspect<AdditionalCMakeOptionsAspect>()->setEnabled(!isParsing);

    // Update label and text boldness of the reconfigure button
    QFont reconfigureButtonFont = m_reconfigureButton->font();
    if (isParsing) {
        m_reconfigureButton->setText(tr("Stop CMake"));
        reconfigureButtonFont.setBold(false);
    } else {
        m_reconfigureButton->setEnabled(true);
        if (isInitial) {
            m_reconfigureButton->setText(tr("Re-configure with Initial Parameters"));
        } else {
            m_reconfigureButton->setText(tr("Run CMake"));
        }
        reconfigureButtonFont.setBold(m_configModel->hasChanges(isInitial));
    }
    m_reconfigureButton->setFont(reconfigureButtonFont);

    m_buildConfiguration->setConfigurationChanges(configChanges);

    // Update the tooltip with the changes
    m_reconfigureButton->setToolTip(
        m_buildConfiguration->configurationChangesArguments(isInitialConfiguration()).join('\n'));
}

void CMakeBuildSettingsWidget::updateAdvancedCheckBox()
{
    if (m_showAdvancedCheckBox->isChecked()) {
        m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
        m_configFilterModel->setFilterRegularExpression("[01]");

    } else {
        m_configFilterModel->setFilterRole(ConfigModel::ItemIsAdvancedRole);
        m_configFilterModel->setFilterFixedString("0");
    }
    updateButtonState();
}

void CMakeBuildSettingsWidget::updateFromKit()
{
    const Kit *k = m_buildConfiguration->kit();
    const CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    // First the key value parameters
    ConfigModel::KitConfiguration configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i);

    m_configModel->setConfigurationFromKit(configHash);

    // Then the additional parameters
    const QStringList additionalKitCMake = ProcessArgs::splitArgs(
        CMakeConfigurationKitAspect::additionalConfiguration(k));
    const QStringList additionalInitialCMake = ProcessArgs::splitArgs(
        m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->value());

    QStringList mergedArgumentList;
    std::set_union(additionalInitialCMake.begin(),
                   additionalInitialCMake.end(),
                   additionalKitCMake.begin(),
                   additionalKitCMake.end(),
                   std::back_inserter(mergedArgumentList));
    m_buildConfiguration->aspect<InitialCMakeArgumentsAspect>()->setValue(
        ProcessArgs::joinArgs(mergedArgumentList));
}

void CMakeBuildSettingsWidget::updateConfigurationStateIndex(int index)
{
    if (index == 0) {
        m_configFilterModel->setFilterRole(ConfigModel::ItemIsInitialRole);
        m_configFilterModel->setFilterFixedString("1");
    } else {
        updateAdvancedCheckBox();
    }

    m_showAdvancedCheckBox->setEnabled(index != 0);

    updateButtonState();
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

    m_setButton->setEnabled(setableCount > 0);
    m_unsetButton->setEnabled(unsetableCount > 0);
    m_editButton->setEnabled(editableCount == 1);
}

void CMakeBuildSettingsWidget::updateConfigurationStateSelection()
{
    const bool hasReplyFile
        = FileApiParser::scanForCMakeReplyFile(m_buildConfiguration->buildDirectory()).exists();

    const int switchToIndex = hasReplyFile ? 1 : 0;
    if (m_configurationStates->currentIndex() != switchToIndex)
        m_configurationStates->setCurrentIndex(switchToIndex);
    else
        emit m_configurationStates->currentChanged(switchToIndex);
}

bool CMakeBuildSettingsWidget::isInitialConfiguration() const
{
    return m_configurationStates->currentIndex() == 0;
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

    auto help = new QAction(tr("Help"), this);
    menu->addAction(help);
    connect(help, &QAction::triggered, this, [=] {
        const CMakeConfigItem item = ConfigModel::dataItemFromIndex(idx).toCMakeConfigItem();

        const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildConfiguration->target()->kit());
        const QString linkUrl = "%1/variable/" + QString::fromUtf8(item.key) + ".html";
        CMakeTool::openCMakeHelpUrl(tool, linkUrl);
    });

    menu->addSeparator();

    QAction *action = nullptr;
    if ((action = createForceAction(ConfigModel::DataItem::BOOLEAN, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::FILE, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::DIRECTORY, idx)))
        menu->addAction(action);
    if ((action = createForceAction(ConfigModel::DataItem::STRING, idx)))
        menu->addAction(action);

    menu->addSeparator();

    auto applyKitOrInitialValue = new QAction(isInitialConfiguration()
                                                  ? tr("Apply Kit Value")
                                                  : tr("Apply Initial Configuration Value"),
                                              this);
    menu->addAction(applyKitOrInitialValue);
    connect(applyKitOrInitialValue, &QAction::triggered, this, [this] {
        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
            return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
        });

        for (const QModelIndex &index : validIndexes) {
            if (isInitialConfiguration())
                m_configModel->applyKitValue(mapToSource(m_configView, index));
            else
                m_configModel->applyInitialValue(mapToSource(m_configView, index));
        }
    });

    menu->addSeparator();

    auto copy = new QAction(tr("Copy"), this);
    menu->addAction(copy);
    connect(copy, &QAction::triggered, this, [this] {
        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
            return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
        });

        const QStringList variableList
            = Utils::transform(validIndexes, [this](const QModelIndex &index) {
                  return ConfigModel::dataItemFromIndex(index).toCMakeConfigItem().toArgument(
                      isInitialConfiguration() ? nullptr : m_buildConfiguration->macroExpander());
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

static bool isWebAssembly(const Kit *k)
{
    return DeviceTypeKitAspect::deviceTypeId(k) == WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE;
}

static bool isQnx(const Kit *k)
{
    return DeviceTypeKitAspect::deviceTypeId(k) == Qnx::Constants::QNX_QNX_OS_TYPE;
}

static bool isDocker(const Kit *k)
{
    return DeviceTypeKitAspect::deviceTypeId(k) == Docker::Constants::DOCKER_DEVICE_TYPE;
}

static bool isWindowsARM64(const Kit *k)
{
    ToolChain *toolchain = ToolChainKitAspect::cxxToolChain(k);
    QTC_ASSERT(toolchain, return false);
    const Abi targetAbi = toolchain->targetAbi();
    return targetAbi.os() == Abi::WindowsOS && targetAbi.architecture() == Abi::ArmArchitecture
            && targetAbi.wordWidth() == 64;
}

static CommandLine defaultInitialCMakeCommand(const Kit *k, const QString buildType)
{
    // Generator:
    CMakeTool *tool = CMakeKitAspect::cmakeTool(k);
    QTC_ASSERT(tool, return {});

    CommandLine cmd{tool->cmakeExecutable()};
    cmd.addArgs(CMakeGeneratorKitAspect::generatorArguments(k));

    // CMAKE_BUILD_TYPE:
    if (!buildType.isEmpty() && !CMakeGeneratorKitAspect::isMultiConfigGenerator(k))
        cmd.addArg("-DCMAKE_BUILD_TYPE:STRING=" + buildType);

    Internal::CMakeSpecificSettings *settings
        = Internal::CMakeProjectPlugin::projectTypeSpecificSettings();

    // Package manager
    if (!isDocker(k) && settings->packageManagerAutoSetup.value()) {
        cmd.addArg("-DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH="
                   "%{IDE:ResourcePath}/package-manager/auto-setup.cmake");
    }

    // Cross-compilation settings:
    if (!isIos(k)) { // iOS handles this differently
        const QString sysRoot = SysRootKitAspect::sysRoot(k).path();
        if (!sysRoot.isEmpty()) {
            cmd.addArg("-DCMAKE_SYSROOT:PATH" + sysRoot);
            if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(k)) {
                const QString targetTriple = tc->originalTargetTriple();
                cmd.addArg("-DCMAKE_C_COMPILER_TARGET:STRING=" + targetTriple);
                cmd.addArg("-DCMAKE_CXX_COMPILER_TARGET:STRING=%1" + targetTriple);
            }
        }
    }

    cmd.addArgs(CMakeConfigurationKitAspect::toArgumentsList(k));
    cmd.addArgs(CMakeConfigurationKitAspect::additionalConfiguration(k), CommandLine::Raw);

    return cmd;
}

} // namespace Internal

// -----------------------------------------------------------------------------
// CMakeBuildConfiguration:
// -----------------------------------------------------------------------------

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    m_buildSystem = new CMakeBuildSystem(this);

    const auto buildDirAspect = aspect<BuildDirectoryAspect>();
    buildDirAspect->setValueAcceptor(
        [](const QString &oldDir, const QString &newDir) -> Utils::optional<QString> {
            if (oldDir.isEmpty())
                return newDir;

            if (QDir(oldDir).exists("CMakeCache.txt") && !QDir(newDir).exists("CMakeCache.txt")) {
                if (QMessageBox::information(
                        Core::ICore::dialogParent(),
                        tr("Changing Build Directory"),
                        tr("Change the build directory to \"%1\" and start with a "
                           "basic CMake configuration?")
                            .arg(newDir),
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
    initialCMakeArgumentsAspect->setMacroExpanderProvider([this] { return macroExpander(); });

    auto additionalCMakeArgumentsAspect = addAspect<AdditionalCMakeOptionsAspect>();
    additionalCMakeArgumentsAspect->setMacroExpanderProvider([this] { return macroExpander(); });

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

    macroExpander()->registerVariable(CMAKE_OSX_ARCHITECTURES_FLAG,
                                      tr("The CMake flag for the architecture on macOS"),
                                      [target] {
                                          if (HostOsInfo::isRunningUnderRosetta()) {
                                              if (auto *qt = QtSupport::QtKitAspect::qtVersion(target->kit())) {
                                                  const Abis abis = qt->qtAbis();
                                                  for (const Abi &abi : abis) {
                                                      if (abi.architecture() == Abi::ArmArchitecture)
                                                          return QLatin1String("-DCMAKE_OSX_ARCHITECTURES=arm64");
                                                  }
                                              }
                                          }
                                          return QLatin1String();
                                      });

    addAspect<SourceDirectoryAspect>();
    addAspect<BuildTypeAspect>();

    appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *k = target->kit();

        CommandLine cmd = defaultInitialCMakeCommand(k, info.typeName);
        setIsMultiConfig(CMakeGeneratorKitAspect::isMultiConfigGenerator(k));

        // Android magic:
        if (DeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
            const auto &bs = buildSteps()->steps().constLast();
            cmd.addArg("-DANDROID_NATIVE_API_LEVEL:STRING="
                   + bs->data(Android::Constants::AndroidNdkPlatform).toString());
            auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
            cmd.addArg("-DANDROID_NDK:PATH=" + ndkLocation.path());

            cmd.addArg("-DCMAKE_TOOLCHAIN_FILE:FILEPATH="
                   + ndkLocation.pathAppended("build/cmake/android.toolchain.cmake").path());

            auto androidAbis = bs->data(Android::Constants::AndroidMkSpecAbis).toStringList();
            QString preferredAbi;
            if (androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A)) {
                preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A;
            } else if (androidAbis.isEmpty()
                       || androidAbis.contains(ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A)) {
                preferredAbi = ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A;
            } else {
                preferredAbi = androidAbis.first();
            }
            cmd.addArg("-DANDROID_ABI:STRING=" + preferredAbi);
            cmd.addArg("-DANDROID_STL:STRING=c++_shared");
            cmd.addArg("-DCMAKE_FIND_ROOT_PATH:PATH=%{Qt:QT_INSTALL_PREFIX}");

            QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();

            if (qt && qt->qtVersion() >= QtSupport::QtVersionNumber{6, 0, 0}) {
                // Don't build apk under ALL target because Qt Creator will handle it
                if (qt->qtVersion() >= QtSupport::QtVersionNumber{6, 1, 0})
                    cmd.addArg("-DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON");
                cmd.addArg("-DQT_HOST_PATH:PATH=%{Qt:QT_HOST_PREFIX}");
                cmd.addArg("-DANDROID_SDK_ROOT:PATH=" + sdkLocation.path());
            } else {
                cmd.addArg("-DANDROID_SDK:PATH=" + sdkLocation.path());
            }
        }

        const IDevice::ConstPtr device = DeviceKitAspect::device(k);
        if (isIos(k)) {
            QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
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
                cmd.addArg(CMAKE_QT6_TOOLCHAIN_FILE_ARG);
                cmd.addArg("-DCMAKE_OSX_ARCHITECTURES:STRING=" + architecture);
                cmd.addArg("-DCMAKE_OSX_SYSROOT:STRING=" + sysroot);
                cmd.addArg("%{" + QLatin1String(DEVELOPMENT_TEAM_FLAG) + "}");
                cmd.addArg("%{" + QLatin1String(PROVISIONING_PROFILE_FLAG) + "}");
            }
        } else if (device && device->osType() == Utils::OsTypeMac) {
            cmd.addArg("%{" + QLatin1String(CMAKE_OSX_ARCHITECTURES_FLAG) + "}");
        }

        if (isWebAssembly(k) || isQnx(k) || isWindowsARM64(k)) {
            const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
            if (qt && qt->qtVersion().majorVersion >= 6)
                cmd.addArg(CMAKE_QT6_TOOLCHAIN_FILE_ARG);
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

        setInitialCMakeArguments(cmd.splitArguments());
        setCMakeBuildType(info.typeName);
    });

    const auto qmlDebuggingAspect = addAspect<QtSupport::QmlDebuggingAspect>();
    qmlDebuggingAspect->setKit(target->kit());
    setIsMultiConfig(CMakeGeneratorKitAspect::isMultiConfigGenerator(target->kit()));
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
        CommandLine cmd = defaultInitialCMakeCommand(kit(), buildTypeName);
        for (const CMakeConfigItem &item : conf)
            cmd.addArg(item.toArgument(macroExpander()));
        setInitialCMakeArguments(cmd.splitArguments());
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
    const FilePath projectDir = Project::projectDirectory(projectFilePath);
    FilePath buildPath = BuildConfiguration::buildDirectoryFromTemplate(projectDir,
        projectFilePath, projectName, k, bcName, buildType, BuildConfiguration::ReplaceSpaces);

    if (CMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
        QString path = buildPath.path();
        path = path.left(path.lastIndexOf(QString("-%1").arg(bcName)));
        buildPath.setPath(path);
    }

    return buildPath;
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

QStringList CMakeBuildConfiguration::configurationChangesArguments(bool initialParameters) const
{
    const QList<CMakeConfigItem> filteredInitials
        = Utils::filtered(m_configurationChanges, [initialParameters](const CMakeConfigItem &ci) {
              return initialParameters ? ci.isInitial : !ci.isInitial;
          });
    return Utils::transform(filteredInitials, &CMakeConfigItem::toArgument);
}

QStringList CMakeBuildConfiguration::initialCMakeArguments() const
{
    return aspect<InitialCMakeArgumentsAspect>()->allValues();
}

CMakeConfig CMakeBuildConfiguration::initialCMakeConfiguration() const
{
    return aspect<InitialCMakeArgumentsAspect>()->cmakeConfiguration();
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
    QStringList additionalArguments;
    aspect<InitialCMakeArgumentsAspect>()->setAllValues(args.join('\n'), additionalArguments);

    // Set the unknown additional arguments also for the "Current Configuration"
    setAdditionalCMakeArguments(additionalArguments);
}

QStringList CMakeBuildConfiguration::additionalCMakeArguments() const
{
    return ProcessArgs::splitArgs(aspect<AdditionalCMakeOptionsAspect>()->value());
}

void CMakeBuildConfiguration::setAdditionalCMakeArguments(const QStringList &args)
{
    const QStringList expandedAdditionalArguments = Utils::transform(args, [this](const QString &s) {
        return macroExpander()->expand(s);
    });
    const QStringList nonEmptyAdditionalArguments = Utils::filtered(expandedAdditionalArguments,
                                                                    [](const QString &s) {
                                                                        return !s.isEmpty();
                                                                    });
    aspect<AdditionalCMakeOptionsAspect>()->setValue(
        ProcessArgs::joinArgs(nonEmptyAdditionalArguments));
}

void CMakeBuildConfiguration::filterConfigArgumentsFromAdditionalCMakeArguments()
{
    // On iOS the %{Ios:DevelopmentTeam:Flag} evalues to something like
    // -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM:STRING=MAGICSTRING
    // which is already part of the CMake variables and should not be also
    // in the addtional CMake options
    const QStringList arguments = ProcessArgs::splitArgs(
        aspect<AdditionalCMakeOptionsAspect>()->value());
    QStringList unknownOptions;
    const CMakeConfig config = CMakeConfig::fromArguments(arguments, unknownOptions);

    aspect<AdditionalCMakeOptionsAspect>()->setValue(ProcessArgs::joinArgs(unknownOptions));
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
    TaskHub::addTask(BuildSystemTask(Task::TaskType::Error, message));
    emit errorOccurred(m_error);
}

void CMakeBuildConfiguration::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    TaskHub::addTask(BuildSystemTask(Task::TaskType::Warning, message));
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
    QByteArray cmakeBuildTypeName = m_configurationFromCMake.valueOf("CMAKE_BUILD_TYPE");
    if (cmakeBuildTypeName.isEmpty()) {
        QByteArray cmakeCfgTypes = m_configurationFromCMake.valueOf("CMAKE_CONFIGURATION_TYPES");
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

void CMakeBuildConfiguration::setSourceDirectory(const FilePath &path)
{
    aspect<SourceDirectoryAspect>()->setFilePath(path);
}

FilePath CMakeBuildConfiguration::sourceDirectory() const
{
    return aspect<SourceDirectoryAspect>()->filePath();
}

QString CMakeBuildConfiguration::cmakeBuildType() const
{
    auto setBuildTypeFromConfig = [this](const CMakeConfig &config) {
        auto it = std::find_if(config.begin(), config.end(), [](const CMakeConfigItem &item) {
            return item.key == "CMAKE_BUILD_TYPE" && !item.isInitial;
        });
        if (it != config.end())
            const_cast<CMakeBuildConfiguration*>(this)
                ->setCMakeBuildType(QString::fromUtf8(it->value));
    };

    if (!isMultiConfig())
        setBuildTypeFromConfig(configurationChanges());

    QString cmakeBuildType = aspect<BuildTypeAspect>()->value();

    const Utils::FilePath cmakeCacheTxt = buildDirectory().pathAppended("CMakeCache.txt");
    const bool hasCMakeCache = QFile::exists(cmakeCacheTxt.toString());
    CMakeConfig config;

    if (cmakeBuildType == "Unknown") {
        // The "Unknown" type is the case of loading of an existing project
        // that doesn't have the "CMake.Build.Type" aspect saved
        if (hasCMakeCache) {
            QString errorMessage;
            config = CMakeBuildSystem::parseCMakeCacheDotTxt(cmakeCacheTxt, &errorMessage);
        } else {
            config = initialCMakeConfiguration();
        }
    } else if (!hasCMakeCache) {
        config = initialCMakeConfiguration();
    }

    if (!config.isEmpty() && !isMultiConfig())
        setBuildTypeFromConfig(config);

    return cmakeBuildType;
}

void CMakeBuildConfiguration::setCMakeBuildType(const QString &cmakeBuildType, bool quiet)
{
    if (quiet) {
        aspect<BuildTypeAspect>()->setValueQuietly(cmakeBuildType);
        aspect<BuildTypeAspect>()->update();
    } else {
        aspect<BuildTypeAspect>()->setValue(cmakeBuildType);
    }
}

bool CMakeBuildConfiguration::isMultiConfig() const
{
    return m_isMultiConfig;
}

void CMakeBuildConfiguration::setIsMultiConfig(bool isMultiConfig)
{
    m_isMultiConfig = isMultiConfig;
}

namespace Internal {

// ----------------------------------------------------------------------
// - InitialCMakeParametersAspect:
// ----------------------------------------------------------------------

const CMakeConfig &InitialCMakeArgumentsAspect::cmakeConfiguration() const
{
    return m_cmakeConfiguration;
}

const QStringList InitialCMakeArgumentsAspect::allValues() const
{
    QStringList initialCMakeArguments = Utils::transform(m_cmakeConfiguration.toList(),
                                                         [](const CMakeConfigItem &ci) {
                                                             return ci.toArgument(nullptr);
                                                         });

    initialCMakeArguments.append(ProcessArgs::splitArgs(value()));

    return initialCMakeArguments;
}

void InitialCMakeArgumentsAspect::setAllValues(const QString &values, QStringList &additionalOptions)
{
    QStringList arguments = values.split('\n', Qt::SkipEmptyParts);
    for (QString &arg: arguments) {
        if (arg.startsWith("-G"))
            arg.replace("-G", "-DCMAKE_GENERATOR:STRING=");
        if (arg.startsWith("-A"))
            arg.replace("-A", "-DCMAKE_GENERATOR_PLATFORM:STRING=");
        if (arg.startsWith("-T"))
            arg.replace("-T", "-DCMAKE_GENERATOR_TOOLSET:STRING=");
    }
    m_cmakeConfiguration = CMakeConfig::fromArguments(arguments, additionalOptions);
    for (CMakeConfigItem &ci : m_cmakeConfiguration)
        ci.isInitial = true;

    // Display the unknown arguments in "Additional CMake Options"
    const QString additionalOptionsValue = ProcessArgs::joinArgs(additionalOptions);
    BaseAspect::setValueQuietly(additionalOptionsValue);
}

void InitialCMakeArgumentsAspect::setCMakeConfiguration(const CMakeConfig &config)
{
    m_cmakeConfiguration = config;
    for (CMakeConfigItem &ci : m_cmakeConfiguration)
        ci.isInitial = true;
}

void InitialCMakeArgumentsAspect::fromMap(const QVariantMap &map)
{
    const QString value = map.value(settingsKey(), defaultValue()).toString();
    QStringList additionalArguments;
    setAllValues(value, additionalArguments);
}

void InitialCMakeArgumentsAspect::toMap(QVariantMap &map) const
{
    saveToMap(map, allValues().join('\n'), defaultValue(), settingsKey());
}

InitialCMakeArgumentsAspect::InitialCMakeArgumentsAspect()
{
    setSettingsKey("CMake.Initial.Parameters");
    setLabelText(tr("Additional CMake <a href=\"options\">options</a>:"));
    setDisplayStyle(LineEditDisplay);
}

// ----------------------------------------------------------------------
// - AdditionalCMakeOptionsAspect:
// ----------------------------------------------------------------------

AdditionalCMakeOptionsAspect::AdditionalCMakeOptionsAspect()
{
    setSettingsKey("CMake.Additional.Options");
    setLabelText(tr("Additional CMake <a href=\"options\">options</a>:"));
    setDisplayStyle(LineEditDisplay);
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
    setLabelText(tr("Build type:"));
    setDisplayStyle(LineEditDisplay);
    setDefaultValue("Unknown");
}

} // namespace Internal
} // namespace CMakeProjectManager
