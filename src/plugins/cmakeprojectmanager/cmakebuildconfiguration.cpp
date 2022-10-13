// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "cmakebuildconfiguration.h"

#include "cmakebuildconfiguration.h"
#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeconfigitem.h"
#include "cmakekitinformation.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakeprojectplugin.h"
#include "cmakespecificsettings.h"
#include "configmodel.h"
#include "configmodelitemdelegate.h"
#include "fileapiparser.h"
#include "presetsmacros.h"
#include "presetsparser.h"

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
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/checkablemessagebox.h>
#include <utils/commandline.h>
#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QBoxLayout>
#include <QCheckBox>
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
const char QT_QML_DEBUG_FLAG[] = "Qt:QML_DEBUG_FLAG";
const char CMAKE_QT6_TOOLCHAIN_FILE_ARG[]
    = "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=%{Qt:QT_INSTALL_PREFIX}/lib/cmake/Qt6/qt.toolchain.cmake";
const char CMAKE_BUILD_TYPE[] = "CMake.Build.Type";
const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "CMake.Configure.ClearSystemEnvironment";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "CMake.Configure.UserEnvironmentChanges";

namespace Internal {

class CMakeBuildSettingsWidget : public NamedWidget
{
public:
    CMakeBuildSettingsWidget(CMakeBuildSystem *bc);

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
    void reconfigureWithInitialParameters();
    void updateInitialCMakeArguments();
    void kitCMakeConfiguration();
    void updateConfigureDetailsWidgetsSummary(
        const QStringList &configurationArguments = QStringList());

    CMakeBuildSystem *m_buildSystem;
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
    DetailsWidget *m_configureDetailsWidget;

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

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildSystem *bs) :
    NamedWidget(Tr::tr("CMake")),
    m_buildSystem(bs),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new CategorySortFilterModel(this)),
    m_configTextFilterModel(new CategorySortFilterModel(this))
{
    QTC_ASSERT(bs, return);
    BuildConfiguration *bc = bs->buildConfiguration();
    CMakeBuildConfiguration *cbc = static_cast<CMakeBuildConfiguration *>(bc);

    m_configureDetailsWidget = new DetailsWidget;

    updateConfigureDetailsWidgetsSummary();

    auto details = new QWidget(m_configureDetailsWidget);
    m_configureDetailsWidget->setWidget(details);

    auto buildDirAspect = bc->buildDirectoryAspect();
    buildDirAspect->setAutoApplyOnEditingFinished(true);
    connect(buildDirAspect, &BaseAspect::changed, this, [this]() {
        m_configModel->flush(); // clear out config cache...;
    });

    auto buildTypeAspect = bc->aspect<BuildTypeAspect>();
    connect(buildTypeAspect, &BaseAspect::changed, this, [this, buildTypeAspect] {
        if (!m_buildSystem->isMultiConfig()) {
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
    m_configurationStates->addTab(Tr::tr("Initial Configuration"));
    m_configurationStates->addTab(Tr::tr("Current Configuration"));
    connect(m_configurationStates, &QTabBar::currentChanged, this, [this](int index) {
        updateConfigurationStateIndex(index);
    });

    m_kitConfiguration = new QPushButton(Tr::tr("Kit Configuration"));
    m_kitConfiguration->setToolTip(Tr::tr("Edit the current kit's CMake configuration."));
    m_kitConfiguration->setFixedWidth(m_kitConfiguration->sizeHint().width());
    connect(m_kitConfiguration, &QPushButton::clicked, this, [this]() { kitCMakeConfiguration(); });

    m_filterEdit = new FancyLineEdit;
    m_filterEdit->setPlaceholderText(Tr::tr("Filter"));
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
    (void) new HeaderViewStretcher(m_configView->header(), 0);
    m_configView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_configView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_configView->setAlternatingRowColors(true);
    m_configView->setFrameShape(QFrame::NoFrame);
    m_configView->setItemDelegate(new ConfigModelItemDelegate(bc->project()->projectDirectory(),
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

    m_addButton = new QPushButton(Tr::tr("&Add"));
    m_addButton->setToolTip(Tr::tr("Add a new configuration value."));
    auto addButtonMenu = new QMenu(this);
    addButtonMenu->addAction(Tr::tr("&Boolean"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::BOOLEAN)));
    addButtonMenu->addAction(Tr::tr("&String"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::STRING)));
    addButtonMenu->addAction(Tr::tr("&Directory"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::DIRECTORY)));
    addButtonMenu->addAction(Tr::tr("&File"))->setData(
                QVariant::fromValue(static_cast<int>(ConfigModel::DataItem::FILE)));
    m_addButton->setMenu(addButtonMenu);

    m_editButton = new QPushButton(Tr::tr("&Edit"));
    m_editButton->setToolTip(Tr::tr("Edit the current CMake configuration value."));

    m_setButton = new QPushButton(Tr::tr("&Set"));
    m_setButton->setToolTip(Tr::tr("Set a value in the CMake configuration."));

    m_unsetButton = new QPushButton(Tr::tr("&Unset"));
    m_unsetButton->setToolTip(Tr::tr("Unset a value in the CMake configuration."));

    m_resetButton = new QPushButton(Tr::tr("&Reset"));
    m_resetButton->setToolTip(Tr::tr("Reset all unapplied changes."));
    m_resetButton->setEnabled(false);

    m_batchEditButton = new QPushButton(Tr::tr("Batch Edit..."));
    m_batchEditButton->setToolTip(Tr::tr("Set or reset multiple values in the CMake configuration."));

    m_showAdvancedCheckBox = new QCheckBox(Tr::tr("Advanced"));

    connect(m_configView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &, const QItemSelection &) {
                updateSelection();
    });

    m_reconfigureButton = new QPushButton(Tr::tr("Run CMake"));
    m_reconfigureButton->setEnabled(false);

    auto clearBox = new QCheckBox(Tr::tr("Clear system environment"), this);
    clearBox->setChecked(cbc->useClearConfigureEnvironment());

    auto envWidget = new EnvironmentWidget(this, EnvironmentWidget::TypeLocal, clearBox);
    envWidget->setBaseEnvironment(cbc->baseConfigureEnvironment());
    envWidget->setBaseEnvironmentText(cbc->baseConfigureEnvironmentText());
    envWidget->setUserChanges(cbc->userConfigureEnvironmentChanges());

    connect(envWidget, &EnvironmentWidget::userChangesChanged, this, [cbc, envWidget] {
        cbc->setUserConfigureEnvironmentChanges(envWidget->userChanges());
    });

    connect(clearBox, &QAbstractButton::toggled, this, [cbc, envWidget](bool checked) {
        cbc->setUseClearConfigureEnvironment(checked);
        envWidget->setBaseEnvironment(cbc->baseConfigureEnvironment());
        envWidget->setBaseEnvironmentText(cbc->baseConfigureEnvironmentText());
    });

    connect(cbc, &CMakeBuildConfiguration::configureEnvironmentChanged, this, [cbc, envWidget] {
        envWidget->setBaseEnvironment(cbc->baseConfigureEnvironment());
        envWidget->setBaseEnvironmentText(cbc->baseConfigureEnvironmentText());
    });

    using namespace Layouting;
    Grid cmakeConfiguration {
        m_filterEdit, br,
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
            st
        }
    };

    Column {
        Form {
            buildDirAspect,
            bc->aspect<BuildTypeAspect>(),
            qmlDebugAspect
        },
        m_warningMessageLabel,
        m_kitConfiguration,
        Column {
            m_configurationStates,
            Group {
                Column {
                    cmakeConfiguration,
                    Row {
                        bc->aspect<InitialCMakeArgumentsAspect>(),
                        bc->aspect<AdditionalCMakeOptionsAspect>()
                    },
                    m_reconfigureButton,
                }
            },
            clearBox,
            envWidget
        }.setSpacing(0)
    }.attachTo(details, WithoutMargins);

    Column {
        m_configureDetailsWidget,
    }.attachTo(this, WithoutMargins);

    updateAdvancedCheckBox();
    setError(m_buildSystem->error());
    setWarning(m_buildSystem->warning());

    connect(m_buildSystem, &BuildSystem::parsingStarted, this, [this] {
        updateButtonState();
        m_configView->setEnabled(false);
        m_showProgressTimer.start();
    });

    m_configModel->setMacroExpander(bc->macroExpander());

    if (m_buildSystem->isParsing())
        m_showProgressTimer.start();
    else {
        m_configModel->setConfiguration(m_buildSystem->configurationFromCMake());
        m_configModel->setInitialParametersConfiguration(
            m_buildSystem->initialCMakeConfiguration());
    }

    connect(m_buildSystem, &BuildSystem::parsingFinished, this, [this] {
        const CMakeConfig config = m_buildSystem->configurationFromCMake();
        auto qmlDebugAspect = m_buildSystem->buildConfiguration()
                                  ->aspect<QtSupport::QmlDebuggingAspect>();
        const TriState qmlDebugSetting = qmlDebugAspect->value();
        bool qmlDebugConfig = CMakeBuildConfiguration::hasQmlDebugging(config);
        if ((qmlDebugSetting == TriState::Enabled && !qmlDebugConfig)
            || (qmlDebugSetting == TriState::Disabled && qmlDebugConfig)) {
            qmlDebugAspect->setValue(TriState::Default);
        }
        m_configModel->setConfiguration(config);
        m_configModel->setInitialParametersConfiguration(
            m_buildSystem->initialCMakeConfiguration());
        m_buildSystem->filterConfigArgumentsFromAdditionalCMakeArguments();
        updateFromKit();
        m_configView->setEnabled(true);
        updateButtonState();
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
        updateConfigurationStateSelection();
    });

    connect(m_buildSystem, &CMakeBuildSystem::configurationCleared, this, [this] {
        updateConfigurationStateSelection();
    });

    connect(m_buildSystem, &CMakeBuildSystem::errorOccurred, this, [this] {
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
        updateConfigurationStateSelection();
    });

    connect(m_configModel, &QAbstractItemModel::dataChanged,
            this, &CMakeBuildSettingsWidget::updateButtonState);
    connect(m_configModel, &QAbstractItemModel::modelReset,
            this, &CMakeBuildSettingsWidget::updateButtonState);

    connect(m_buildSystem->cmakeBuildConfiguration(),
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
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this] {
        if (!m_buildSystem->isParsing()) {
            if (isInitialConfiguration()) {
                reconfigureWithInitialParameters();
            } else {
                m_buildSystem->runCMakeWithExtraArguments();
            }
        } else {
            m_buildSystem->stopCMakeRun();
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
        QString value = Tr::tr("<UNSET>");
        if (type == ConfigModel::DataItem::BOOLEAN)
            value = QString::fromLatin1("OFF");

        m_configModel->appendConfiguration(Tr::tr("<UNSET>"), value, type, isInitialConfiguration());
        const TreeItem *item = m_configModel->findNonRootItem([&value, type](TreeItem *item) {
                ConfigModel::DataItem dataItem = ConfigModel::dataItemFromIndex(item->index());
                return dataItem.key == Tr::tr("<UNSET>") && dataItem.type == type && dataItem.value == value;
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

    connect(m_buildSystem, &CMakeBuildSystem::errorOccurred,
            this, &CMakeBuildSettingsWidget::setError);
    connect(m_buildSystem, &CMakeBuildSystem::warningOccurred,
            this, &CMakeBuildSettingsWidget::setWarning);

    connect(m_buildSystem, &CMakeBuildSystem::configurationChanged,
            m_configModel, &ConfigModel::setBatchEditConfiguration);

    updateFromKit();
    connect(m_buildSystem->target(), &Target::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(bc, &CMakeBuildConfiguration::enabledChanged, this, [this, bc] {
        if (bc->isEnabled())
            setError(QString());
    });
    connect(this, &QObject::destroyed, this, [this] {
        updateInitialCMakeArguments();
    });

    connect(bc->aspect<InitialCMakeArgumentsAspect>(),
            &Utils::BaseAspect::labelLinkActivated,
            this,
            [this](const QString &) {
                const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildSystem->kit());
                CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake.1.html#options");
            });
    connect(bc->aspect<AdditionalCMakeOptionsAspect>(),
            &Utils::BaseAspect::labelLinkActivated, this, [this](const QString &) {
                const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildSystem->kit());
                CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake.1.html#options");
            });

    updateSelection();
    updateConfigurationStateSelection();
}

void CMakeBuildSettingsWidget::batchEditConfiguration()
{
    auto dialog = new QDialog(this);
    dialog->setWindowTitle(Tr::tr("Edit CMake Configuration"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    auto layout = new QVBoxLayout(dialog);
    auto editor = new QPlainTextEdit(dialog);

    auto label = new QLabel(dialog);
    label->setText(Tr::tr("Enter one CMake <a href=\"variable\">variable</a> per line.<br/>"
       "To set or change a variable, use -D&lt;variable&gt;:&lt;type&gt;=&lt;value&gt;.<br/>"
       "&lt;type&gt; can have one of the following values: FILEPATH, PATH, BOOL, INTERNAL, or STRING.<br/>"
                      "To unset a variable, use -U&lt;variable&gt;.<br/>"));
    connect(label, &QLabel::linkActivated, this, [this](const QString &) {
        const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildSystem->target()->kit());
        CMakeTool::openCMakeHelpUrl(tool, "%1/manual/cmake-variables.7.html");
    });
    editor->setMinimumSize(800, 200);

    auto chooser = new Utils::VariableChooser(dialog);
    chooser->addSupportedWidget(editor);
    chooser->addMacroExpanderProvider([this] { return m_buildSystem->buildConfiguration()->macroExpander(); });

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    layout->addWidget(editor);
    layout->addWidget(label);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::accepted, this, [=]{
        const auto expander = m_buildSystem->buildConfiguration()->macroExpander();

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
        m_buildSystem->configurationChangesArguments(isInitialConfiguration())
            .join('\n'));

    dialog->show();
}

void CMakeBuildSettingsWidget::reconfigureWithInitialParameters()
{
    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    bool doNotAsk = !settings->askBeforeReConfigureInitialParams.value();
    if (!doNotAsk) {
        QDialogButtonBox::StandardButton reply = Utils::CheckableMessageBox::question(
            Core::ICore::dialogParent(),
            Tr::tr("Re-configure with Initial Parameters"),
            Tr::tr("Clear CMake configuration and configure with initial parameters?"),
            Tr::tr("Do not ask again"),
            &doNotAsk,
            QDialogButtonBox::Yes | QDialogButtonBox::No,
            QDialogButtonBox::Yes);

        settings->askBeforeReConfigureInitialParams.setValue(!doNotAsk);
        settings->writeSettings(Core::ICore::settings());

        if (reply != QDialogButtonBox::Yes) {
            return;
        }
    }

    m_buildSystem->clearCMakeCache();

    updateInitialCMakeArguments();

    if (ProjectExplorerPlugin::saveModifiedFiles())
        m_buildSystem->runCMake();
}

void CMakeBuildSettingsWidget::updateInitialCMakeArguments()
{
    CMakeConfig initialList = m_buildSystem->initialCMakeConfiguration();

    for (const CMakeConfigItem &ci : m_buildSystem->configurationChanges()) {
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
        } else if (!ci.key.isEmpty()) {
            initialList.push_back(ci);
        }
    }

    auto bc = m_buildSystem->buildConfiguration();
    bc->aspect<InitialCMakeArgumentsAspect>()->setCMakeConfiguration(initialList);

    // value() will contain only the unknown arguments (the non -D/-U arguments)
    // As the user would expect to have e.g. "--preset" from "Initial Configuration"
    // to "Current Configuration" as additional parameters
    m_buildSystem->setAdditionalCMakeArguments(ProcessArgs::splitArgs(
        bc->aspect<InitialCMakeArgumentsAspect>()->value()));
}

void CMakeBuildSettingsWidget::kitCMakeConfiguration()
{
    m_buildSystem->kit()->blockNotification();

    auto dialog = new QDialog(this);
    dialog->setWindowTitle(Tr::tr("Kit CMake Configuration"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->setSizeGripEnabled(true);
    connect(dialog, &QDialog::finished, this, [this] {
        m_buildSystem->kit()->unblockNotification();
    });

    CMakeKitAspect kitAspect;
    CMakeGeneratorKitAspect generatorAspect;
    CMakeConfigurationKitAspect configurationKitAspect;

    auto layout = new QGridLayout(dialog);

    kitAspect.createConfigWidget(m_buildSystem->kit())
        ->addToLayoutWithLabel(layout->parentWidget());
    generatorAspect.createConfigWidget(m_buildSystem->kit())
        ->addToLayoutWithLabel(layout->parentWidget());
    configurationKitAspect.createConfigWidget(m_buildSystem->kit())
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

void CMakeBuildSettingsWidget::updateConfigureDetailsWidgetsSummary(
    const QStringList &configurationArguments)
{
    ProjectExplorer::ProcessParameters params;

    CommandLine cmd;
    const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildSystem->kit());
    cmd.setExecutable(tool ? tool->cmakeExecutable() : "cmake");

    const BuildConfiguration *bc = m_buildSystem->buildConfiguration();
    const FilePath buildDirectory = bc ? bc->buildDirectory() : ".";

    cmd.addArgs({"-S", m_buildSystem->projectDirectory().path()});
    cmd.addArgs({"-B", buildDirectory.onDevice(cmd.executable()).path()});
    cmd.addArgs(configurationArguments);

    params.setCommandLine(cmd);
    m_configureDetailsWidget->setSummaryText(params.summary(Tr::tr("Configure")));
    m_configureDetailsWidget->setState(DetailsWidget::Expanded);
}

void CMakeBuildSettingsWidget::setError(const QString &message)
{
    m_buildSystem->buildConfiguration()->buildDirectoryAspect()->setProblem(message);
}

void CMakeBuildSettingsWidget::setWarning(const QString &message)
{
    bool showWarning = !message.isEmpty();
    m_warningMessageLabel->setVisible(showWarning);
    m_warningMessageLabel->setText(message);
}

void CMakeBuildSettingsWidget::updateButtonState()
{
    const bool isParsing = m_buildSystem->isParsing();

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

    BuildConfiguration *bc = m_buildSystem->buildConfiguration();
    bc->aspect<InitialCMakeArgumentsAspect>()->setVisible(isInitialConfiguration());
    bc->aspect<AdditionalCMakeOptionsAspect>()->setVisible(!isInitialConfiguration());

    bc->aspect<InitialCMakeArgumentsAspect>()->setEnabled(!isParsing);
    bc->aspect<AdditionalCMakeOptionsAspect>()->setEnabled(!isParsing);

    // Update label and text boldness of the reconfigure button
    QFont reconfigureButtonFont = m_reconfigureButton->font();
    if (isParsing) {
        m_reconfigureButton->setText(Tr::tr("Stop CMake"));
        reconfigureButtonFont.setBold(false);
    } else {
        m_reconfigureButton->setEnabled(true);
        if (isInitial) {
            m_reconfigureButton->setText(Tr::tr("Re-configure with Initial Parameters"));
        } else {
            m_reconfigureButton->setText(Tr::tr("Run CMake"));
        }
        reconfigureButtonFont.setBold(isInitial ? m_configModel->hasChanges(isInitial)
                                                : !configChanges.isEmpty());
    }
    m_reconfigureButton->setFont(reconfigureButtonFont);

    m_buildSystem->setConfigurationChanges(configChanges);

    // Update the tooltip with the changes
    const QStringList configurationArguments = m_buildSystem->configurationChangesArguments(
        isInitialConfiguration());
    m_reconfigureButton->setToolTip(configurationArguments.join('\n'));
    updateConfigureDetailsWidgetsSummary(configurationArguments);
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
    const Kit *k = m_buildSystem->kit();
    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    config.append(CMakeGeneratorKitAspect::generatorCMakeConfig(k));

    // First the key value parameters
    ConfigModel::KitConfiguration configHash;
    for (const CMakeConfigItem &i : config)
        configHash.insert(QString::fromUtf8(i.key), i);

    m_configModel->setConfigurationFromKit(configHash);

    // Then the additional parameters
    const QStringList additionalKitCMake = ProcessArgs::splitArgs(
        CMakeConfigurationKitAspect::additionalConfiguration(k));
    const QStringList additionalInitialCMake = ProcessArgs::splitArgs(
        m_buildSystem->buildConfiguration()->aspect<InitialCMakeArgumentsAspect>()->value());

    QStringList mergedArgumentList;
    std::set_union(additionalInitialCMake.begin(),
                   additionalInitialCMake.end(),
                   additionalKitCMake.begin(),
                   additionalKitCMake.end(),
                   std::back_inserter(mergedArgumentList));
    m_buildSystem->buildConfiguration()->aspect<InitialCMakeArgumentsAspect>()->setValue(
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
    const auto aspect = m_buildSystem->buildConfiguration()->aspect<QtSupport::QmlDebuggingAspect>();
    const TriState qmlDebuggingState = aspect->value();
    if (qmlDebuggingState == TriState::Default) // don't touch anything
        return {};
    const bool enable = aspect->value() == TriState::Enabled;

    const CMakeConfig configList = m_buildSystem->configurationFromCMake();
    const QByteArrayList cxxFlagsPrev{"CMAKE_CXX_FLAGS",
                                      "CMAKE_CXX_FLAGS_DEBUG",
                                      "CMAKE_CXX_FLAGS_RELWITHDEBINFO",
                                      "CMAKE_CXX_FLAGS_INIT"};
    const QByteArrayList cxxFlags{"CMAKE_CXX_FLAGS_INIT", "CMAKE_CXX_FLAGS"};
    const QByteArray qmlDebug("-DQT_QML_DEBUG");

    CMakeConfig changedConfig;

    if (enable) {
        const FilePath cmakeCache = m_buildSystem->cmakeBuildConfiguration()->buildDirectory().pathAppended("CMakeCache.txt");

        // Only modify the CMAKE_CXX_FLAGS variable if the project was previously configured
        // otherwise CMAKE_CXX_FLAGS_INIT will take care of setting the qmlDebug define
        if (cmakeCache.exists()) {
            for (const CMakeConfigItem &item : configList) {
                if (!cxxFlags.contains(item.key))
                    continue;

                CMakeConfigItem it(item);
                if (!it.value.contains(qmlDebug)) {
                    it.value = it.value.append(' ').append(qmlDebug).trimmed();
                    changedConfig.append(it);
                }
            }
        }
    } else {
        // Remove -DQT_QML_DEBUG from all configurations, potentially set by previous Qt Creator versions
        for (const CMakeConfigItem &item : configList) {
            if (!cxxFlagsPrev.contains(item.key))
                continue;

            CMakeConfigItem it(item);
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
    const CMakeConfig flags = m_buildSystem->cmakeBuildConfiguration()->signingFlags();
    if (flags.isEmpty())
        return {};
    const CMakeConfig configList = m_buildSystem->configurationFromCMake();
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
        = FileApiParser::scanForCMakeReplyFile(
                m_buildSystem->buildConfiguration()->buildDirectory()).exists();

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
        typeString = Tr::tr("bool", "display string for cmake type BOOLEAN");
        break;
    case ConfigModel::DataItem::FILE:
        typeString = Tr::tr("file", "display string for cmake type FILE");
        break;
    case ConfigModel::DataItem::DIRECTORY:
        typeString = Tr::tr("directory", "display string for cmake type DIRECTORY");
        break;
    case ConfigModel::DataItem::STRING:
        typeString = Tr::tr("string", "display string for cmake type STRING");
        break;
    case ConfigModel::DataItem::UNKNOWN:
        return nullptr;
    }
    QAction *forceAction = new QAction(Tr::tr("Force to %1").arg(typeString), nullptr);
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

    auto help = new QAction(Tr::tr("Help"), this);
    menu->addAction(help);
    connect(help, &QAction::triggered, this, [=] {
        const CMakeConfigItem item = ConfigModel::dataItemFromIndex(idx).toCMakeConfigItem();

        const CMakeTool *tool = CMakeKitAspect::cmakeTool(m_buildSystem->target()->kit());
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
                                                  ? Tr::tr("Apply Kit Value")
                                                  : Tr::tr("Apply Initial Configuration Value"),
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

    auto copy = new QAction(Tr::tr("Copy"), this);
    menu->addAction(copy);
    connect(copy, &QAction::triggered, this, [this] {
        const QModelIndexList selectedIndexes = m_configView->selectionModel()->selectedIndexes();

        const QModelIndexList validIndexes = Utils::filtered(selectedIndexes, [](const QModelIndex &index) {
            return index.isValid() && index.flags().testFlag(Qt::ItemIsSelectable);
        });

        const QStringList variableList
            = Utils::transform(validIndexes, [this](const QModelIndex &index) {
                  return ConfigModel::dataItemFromIndex(index).toCMakeConfigItem().toArgument(
                      isInitialConfiguration() ? nullptr
                                               : m_buildSystem->buildConfiguration()->macroExpander());
              });

        setClipboardAndSelection(variableList.join('\n'));
    });

    menu->move(e->globalPos());
    menu->show();

    return true;
}

static bool isWebAssembly(const Kit *k)
{
    return DeviceTypeKitAspect::deviceTypeId(k) == WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE;
}

static bool isQnx(const Kit *k)
{
    return DeviceTypeKitAspect::deviceTypeId(k) == Qnx::Constants::QNX_QNX_OS_TYPE;
}

static bool isWindowsARM64(const Kit *k)
{
    ToolChain *toolchain = ToolChainKitAspect::cxxToolChain(k);
    if (!toolchain)
        return false;
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

    // Package manager auto setup. The file auto-setup.cmake resides on the host,
    // so it's not accessible for remotely running cmakes. We need to exclude that case.
    if (!cmd.executable().needsDevice() && settings->packageManagerAutoSetup.value()) {
        cmd.addArg("-DCMAKE_PROJECT_INCLUDE_BEFORE:FILEPATH="
                   "%{IDE:ResourcePath}/package-manager/auto-setup.cmake");
    }

    // Cross-compilation settings:
    if (!CMakeBuildConfiguration::isIos(k)) { // iOS handles this differently
        const QString sysRoot = SysRootKitAspect::sysRoot(k).path();
        if (!sysRoot.isEmpty()) {
            cmd.addArg("-DCMAKE_SYSROOT:PATH=" + sysRoot);
            if (ToolChain *tc = ToolChainKitAspect::cxxToolChain(k)) {
                const QString targetTriple = tc->originalTargetTriple();
                cmd.addArg("-DCMAKE_C_COMPILER_TARGET:STRING=" + targetTriple);
                cmd.addArg("-DCMAKE_CXX_COMPILER_TARGET:STRING=" + targetTriple);
            }
        }
    }

    cmd.addArgs(CMakeConfigurationKitAspect::toArgumentsList(k));
    cmd.addArgs(CMakeConfigurationKitAspect::additionalConfiguration(k), CommandLine::Raw);

    return cmd;
}

static void addCMakeConfigurePresetToInitialArguments(QStringList &initialArguments,
                                                      const CMakeProject *project,
                                                      const Kit *k,
                                                      const Utils::Environment &env,
                                                      const Utils::FilePath &buildDirectory)

{
    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
    if (presetItem.isNull())
        return;

    // Remove the -DQTC_CMAKE_PRESET argument, which is only used as a kit marker
    const QString presetArgument = presetItem.toArgument();
    const QString presetName = presetItem.expandedValue(k);
    initialArguments.removeIf(
        [presetArgument](const QString &item) { return item == presetArgument; });

    PresetsDetails::ConfigurePreset configurePreset
        = Utils::findOrDefault(project->presetsData().configurePresets,
                               [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                   return preset.name == presetName;
                               });

    // Add the command line arguments
    if (configurePreset.warnings) {
        if (configurePreset.warnings.value().dev) {
            bool value = configurePreset.warnings.value().dev.value();
            initialArguments.append(value ? QString("-Wdev") : QString("-Wno-dev"));
        }
        if (configurePreset.warnings.value().deprecated) {
            bool value = configurePreset.warnings.value().deprecated.value();
            initialArguments.append(value ? QString("-Wdeprecated") : QString("-Wno-deprecated"));
        }
        if (configurePreset.warnings.value().uninitialized
            && configurePreset.warnings.value().uninitialized.value())
            initialArguments.append("--warn-uninitialized");
        if (configurePreset.warnings.value().unusedCli
            && !configurePreset.warnings.value().unusedCli.value())
            initialArguments.append(" --no-warn-unused-cli");
        if (configurePreset.warnings.value().systemVars
            && configurePreset.warnings.value().systemVars.value())
            initialArguments.append("--check-system-vars");
    }

    if (configurePreset.errors) {
        if (configurePreset.errors.value().dev) {
            bool value = configurePreset.errors.value().dev.value();
            initialArguments.append(value ? QString("-Werror=dev") : QString("-Wno-error=dev"));
        }
        if (configurePreset.errors.value().deprecated) {
            bool value = configurePreset.errors.value().deprecated.value();
            initialArguments.append(value ? QString("-Werror=deprecated")
                                          : QString("-Wno-error=deprecated"));
        }
    }

    if (configurePreset.debug) {
        if (configurePreset.debug.value().find && configurePreset.debug.value().find.value())
            initialArguments.append("--debug-find");
        if (configurePreset.debug.value().tryCompile
            && configurePreset.debug.value().tryCompile.value())
            initialArguments.append("--debug-trycompile");
        if (configurePreset.debug.value().output && configurePreset.debug.value().output.value())
            initialArguments.append("--debug-output");
    }

    CMakePresets::Macros::updateToolchainFile(configurePreset,
                                              env,
                                              project->projectDirectory(),
                                              buildDirectory);

    // Merge the presets cache variables
    CMakeConfig cache;
    if (configurePreset.cacheVariables)
        cache = configurePreset.cacheVariables.value();

    for (const CMakeConfigItem &presetItemRaw : cache) {

        // Expand the CMakePresets Macros
        CMakeConfigItem presetItem(presetItemRaw);

        QString presetItemValue = QString::fromUtf8(presetItem.value);
        CMakePresets::Macros::expand(configurePreset, env, project->projectDirectory(), presetItemValue);
        presetItem.value = presetItemValue.toUtf8();

        const QString presetItemArg = presetItem.toArgument();
        const QString presetItemArgNoType = presetItemArg.left(presetItemArg.indexOf(":"));

        auto it = std::find_if(initialArguments.begin(),
                               initialArguments.end(),
                               [presetItemArgNoType](const QString &arg) {
                                   return arg.startsWith(presetItemArgNoType);
                               });

        if (it != initialArguments.end()) {
            QString &arg = *it;
            CMakeConfigItem argItem = CMakeConfigItem::fromString(arg.mid(2)); // skip -D

            // For multi value path variables append the non Qt path
            if (argItem.key == "CMAKE_PREFIX_PATH" || argItem.key == "CMAKE_FIND_ROOT_PATH") {
                QStringList presetValueList = presetItem.expandedValue(k).split(";");

                // Remove the expanded Qt path from the presets values
                QString argItemExpandedValue = argItem.expandedValue(k);
                presetValueList.removeIf([argItemExpandedValue](const QString &presetPath) {
                    QStringList argItemPaths = argItemExpandedValue.split(";");
                    for (const QString &argPath : argItemPaths) {
                        const FilePath argFilePath = FilePath::fromString(argPath);
                        const FilePath presetFilePath = FilePath::fromString(presetPath);

                        if (argFilePath == presetFilePath)
                            return true;
                    }
                    return false;
                });

                // Add the presets values to the final argument
                for (const QString &presetPath : presetValueList) {
                    argItem.value.append(";");
                    argItem.value.append(presetPath.toUtf8());
                }

                arg = argItem.toArgument();
            } else if (argItem.key == "CMAKE_C_COMPILER" || argItem.key == "CMAKE_CXX_COMPILER"
                       || argItem.key == "QT_QMAKE_EXECUTABLE" || argItem.key == "QT_HOST_PATH"
                       || argItem.key == "CMAKE_PROJECT_INCLUDE_BEFORE"
                       || argItem.key == "CMAKE_TOOLCHAIN_FILE") {
                const FilePath argFilePath = FilePath::fromString(argItem.expandedValue(k));
                const FilePath presetFilePath = FilePath::fromUtf8(presetItem.value);

                if (argFilePath != presetFilePath)
                    arg = presetItem.toArgument();
            } else if (argItem.expandedValue(k) != QString::fromUtf8(presetItem.value)) {
                arg = presetItem.toArgument();
            }
        } else {
            initialArguments.append(presetItem.toArgument());
        }
    }
}

static Utils::EnvironmentItems getEnvironmentItemsFromCMakeConfigurePreset(
    const CMakeProject *project, const Kit *k)

{
    Utils::EnvironmentItems envItems;

    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
    if (presetItem.isNull())
        return envItems;

    const QString presetName = presetItem.expandedValue(k);

    PresetsDetails::ConfigurePreset configurePreset
        = Utils::findOrDefault(project->presetsData().configurePresets,
                               [presetName](const PresetsDetails::ConfigurePreset &preset) {
                                   return preset.name == presetName;
                               });

    CMakePresets::Macros::expand(configurePreset, envItems, project->projectDirectory());

    return envItems;
}

static Utils::EnvironmentItems getEnvironmentItemsFromCMakeBuildPreset(
    const CMakeProject *project, const Kit *k, const QString &buildPresetName)

{
    Utils::EnvironmentItems envItems;

    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
    if (presetItem.isNull())
        return envItems;

    PresetsDetails::BuildPreset buildPreset
        = Utils::findOrDefault(project->presetsData().buildPresets,
                               [buildPresetName](const PresetsDetails::BuildPreset &preset) {
                                   return preset.name == buildPresetName;
                               });

    CMakePresets::Macros::expand(buildPreset, envItems, project->projectDirectory());

    return envItems;
}

// -----------------------------------------------------------------------------
// CMakeBuildConfigurationPrivate:
// -----------------------------------------------------------------------------

class CMakeBuildConfigurationPrivate
{
public:
    Utils::Environment m_configureEnvironment;
    Utils::EnvironmentItems  m_userConfigureEnvironmentChanges;
    bool m_clearSystemConfigureEnvironment = false;
};

} // namespace Internal

// -----------------------------------------------------------------------------
// CMakeBuildConfiguration:
// -----------------------------------------------------------------------------

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id), d(new Internal::CMakeBuildConfigurationPrivate())
{
    m_buildSystem = new CMakeBuildSystem(this);

    const auto buildDirAspect = aspect<BuildDirectoryAspect>();
    buildDirAspect->setValueAcceptor(
        [](const QString &oldDir, const QString &newDir) -> std::optional<QString> {
            if (oldDir.isEmpty())
                return newDir;

            if (QDir(oldDir).exists("CMakeCache.txt") && !QDir(newDir).exists("CMakeCache.txt")) {
                if (QMessageBox::information(
                        Core::ICore::dialogParent(),
                        Tr::tr("Changing Build Directory"),
                        Tr::tr("Change the build directory to \"%1\" and start with a "
                           "basic CMake configuration?")
                            .arg(newDir),
                        QMessageBox::Ok,
                        QMessageBox::Cancel)
                    == QMessageBox::Ok) {
                    return newDir;
                }
                return std::nullopt;
            }
            return newDir;
        });

    auto initialCMakeArgumentsAspect = addAspect<InitialCMakeArgumentsAspect>();
    initialCMakeArgumentsAspect->setMacroExpanderProvider([this] { return macroExpander(); });

    auto additionalCMakeArgumentsAspect = addAspect<AdditionalCMakeOptionsAspect>();
    additionalCMakeArgumentsAspect->setMacroExpanderProvider([this] { return macroExpander(); });

    macroExpander()->registerVariable(DEVELOPMENT_TEAM_FLAG,
                                      Tr::tr("The CMake flag for the development team"),
                                      [this] {
                                          const CMakeConfig flags = signingFlags();
                                          if (!flags.isEmpty())
                                              return flags.first().toArgument();
                                          return QString();
                                      });
    macroExpander()->registerVariable(PROVISIONING_PROFILE_FLAG,
                                      Tr::tr("The CMake flag for the provisioning profile"),
                                      [this] {
                                          const CMakeConfig flags = signingFlags();
                                          if (flags.size() > 1 && !flags.at(1).isUnset) {
                                              return flags.at(1).toArgument();
                                          }
                                          return QString();
                                      });

    macroExpander()->registerVariable(CMAKE_OSX_ARCHITECTURES_FLAG,
                                      Tr::tr("The CMake flag for the architecture on macOS"),
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
    macroExpander()->registerVariable(QT_QML_DEBUG_FLAG,
                                      Tr::tr("The CMake flag for QML debugging, if enabled"),
                                      [this] {
                                          if (aspect<QtSupport::QmlDebuggingAspect>()->value()
                                              == TriState::Enabled) {
                                              return QLatin1String(
                                                  "-DQT_QML_DEBUG");
                                          }
                                          return QLatin1String();
                                      });

    addAspect<SourceDirectoryAspect>();
    addAspect<BuildTypeAspect>();
    addAspect<QtSupport::QmlDebuggingAspect>(this);

    setInitialBuildAndCleanSteps(target);

    setInitializer([this, target](const BuildInfo &info) {
        const Kit *k = target->kit();
        const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
        const QVariantMap extraInfoMap = info.extraInfo.value<QVariantMap>();
        const QString buildType = extraInfoMap.contains(CMAKE_BUILD_TYPE)
                                      ? extraInfoMap.value(CMAKE_BUILD_TYPE).toString()
                                      : info.typeName;
        const TriState qmlDebugging = extraInfoMap.contains(Constants::QML_DEBUG_SETTING)
                                          ? TriState::fromVariant(
                                              extraInfoMap.value(Constants::QML_DEBUG_SETTING))
                                          : TriState::Default;

        CommandLine cmd = defaultInitialCMakeCommand(k, buildType);
        m_buildSystem->setIsMultiConfig(CMakeGeneratorKitAspect::isMultiConfigGenerator(k));

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

            auto sdkLocation = bs->data(Android::Constants::SdkLocation).value<FilePath>();

            if (qt && qt->qtVersion() >= QVersionNumber(6, 0, 0)) {
                // Don't build apk under ALL target because Qt Creator will handle it
                if (qt->qtVersion() >= QVersionNumber(6, 1, 0))
                    cmd.addArg("-DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON");
                cmd.addArg("-DQT_HOST_PATH:PATH=%{Qt:QT_HOST_PREFIX}");
                cmd.addArg("-DANDROID_SDK_ROOT:PATH=" + sdkLocation.path());
            } else {
                cmd.addArg("-DANDROID_SDK:PATH=" + sdkLocation.path());
            }
        }

        const IDevice::ConstPtr device = DeviceKitAspect::device(k);
        if (CMakeBuildConfiguration::isIos(k)) {
            if (qt && qt->qtVersion().majorVersion() >= 6) {
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
            if (qt && qt->qtVersion().majorVersion() >= 6)
                cmd.addArg(CMAKE_QT6_TOOLCHAIN_FILE_ARG);
        }

        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.typeName,
                                                   info.buildType));
        }

        if (extraInfoMap.contains(Constants::CMAKE_HOME_DIR))
            setSourceDirectory(FilePath::fromVariant(extraInfoMap.value(Constants::CMAKE_HOME_DIR)));

        aspect<QtSupport::QmlDebuggingAspect>()->setValue(qmlDebugging);

        if (qt && qt->isQmlDebuggingSupported())
            cmd.addArg("-DCMAKE_CXX_FLAGS_INIT:STRING=%{" + QLatin1String(QT_QML_DEBUG_FLAG) + "}");

        CMakeProject *cmakeProject = static_cast<CMakeProject *>(target->project());
        setUserConfigureEnvironmentChanges(getEnvironmentItemsFromCMakeConfigurePreset(cmakeProject, k));
        updateAndEmitConfigureEnvironmentChanged();

        QStringList initialCMakeArguments = cmd.splitArguments();
        addCMakeConfigurePresetToInitialArguments(initialCMakeArguments,
                                                  cmakeProject,
                                                  k,
                                                  configureEnvironment(),
                                                  info.buildDirectory);
        m_buildSystem->setInitialCMakeArguments(initialCMakeArguments);
        m_buildSystem->setCMakeBuildType(buildType);

        setBuildPresetToBuildSteps(target);
    });
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_buildSystem;
    delete d;
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), d->m_clearSystemConfigureEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), EnvironmentItem::toStringList(d->m_userConfigureEnvironmentChanges));
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
    if (m_buildSystem->initialCMakeArguments().isEmpty()) {
        CommandLine cmd = defaultInitialCMakeCommand(kit(), buildTypeName);
        for (const CMakeConfigItem &item : conf)
            cmd.addArg(item.toArgument(macroExpander()));
        m_buildSystem->setInitialCMakeArguments(cmd.splitArguments());
    }

    d->m_clearSystemConfigureEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY))
                                               .toBool();
    d->m_userConfigureEnvironmentChanges = EnvironmentItem::fromStringList(
        map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    updateAndEmitConfigureEnvironmentChanged();

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
    FilePath buildPath = buildDirectoryFromTemplate(projectDir, projectFilePath, projectName, k,
                                                    bcName, buildType, "cmake");

    if (CMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
        const QString path = buildPath.path();
        buildPath = buildPath.withNewPath(path.left(path.lastIndexOf(QString("-%1").arg(bcName))));
    }

    return buildPath;
}

bool CMakeBuildConfiguration::isIos(const Kit *k)
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

bool CMakeBuildConfiguration::hasQmlDebugging(const CMakeConfig &config)
{
    // Determine QML debugging flags. This must match what we do in
    // CMakeBuildSettingsWidget::getQmlDebugCxxFlags()
    // such that in doubt we leave the QML Debugging setting at "Leave at default"
    const QString cxxFlagsInit = config.stringValueOf("CMAKE_CXX_FLAGS_INIT");
    const QString cxxFlags = config.stringValueOf("CMAKE_CXX_FLAGS");
    return cxxFlagsInit.contains("-DQT_QML_DEBUG") && cxxFlags.contains("-DQT_QML_DEBUG");
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

CMakeConfig CMakeBuildSystem::configurationFromCMake() const
{
    return m_configurationFromCMake;
}

CMakeConfig CMakeBuildSystem::configurationChanges() const
{
    return m_configurationChanges;
}

QStringList CMakeBuildSystem::configurationChangesArguments(bool initialParameters) const
{
    const QList<CMakeConfigItem> filteredInitials
        = Utils::filtered(m_configurationChanges, [initialParameters](const CMakeConfigItem &ci) {
              return initialParameters ? ci.isInitial : !ci.isInitial;
          });
    return Utils::transform(filteredInitials, &CMakeConfigItem::toArgument);
}

QStringList CMakeBuildSystem::initialCMakeArguments() const
{
    return buildConfiguration()->aspect<InitialCMakeArgumentsAspect>()->allValues();
}

CMakeConfig CMakeBuildSystem::initialCMakeConfiguration() const
{
    return buildConfiguration()->aspect<InitialCMakeArgumentsAspect>()->cmakeConfiguration();
}

void CMakeBuildSystem::setConfigurationFromCMake(const CMakeConfig &config)
{
    m_configurationFromCMake = config;
}

void CMakeBuildSystem::setConfigurationChanges(const CMakeConfig &config)
{
    qCDebug(cmakeBuildConfigurationLog)
        << "Configuration changes before:" << configurationChangesArguments();

    m_configurationChanges = config;

    qCDebug(cmakeBuildConfigurationLog)
        << "Configuration changes after:" << configurationChangesArguments();
}

// FIXME: Run clean steps when a setting starting with "ANDROID_BUILD_ABI_" is changed.
// FIXME: Warn when kit settings are overridden by a project.

void CMakeBuildSystem::clearError(ForceEnabledChanged fec)
{
    if (!m_error.isEmpty()) {
        m_error.clear();
        fec = ForceEnabledChanged::True;
    }
    if (fec == ForceEnabledChanged::True) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit buildConfiguration()->enabledChanged();
    }
}

void CMakeBuildSystem::setInitialCMakeArguments(const QStringList &args)
{
    QStringList additionalArguments;
    buildConfiguration()->aspect<InitialCMakeArgumentsAspect>()->setAllValues(args.join('\n'), additionalArguments);

    // Set the unknown additional arguments also for the "Current Configuration"
    setAdditionalCMakeArguments(additionalArguments);
}

QStringList CMakeBuildSystem::additionalCMakeArguments() const
{
    return ProcessArgs::splitArgs(buildConfiguration()->aspect<AdditionalCMakeOptionsAspect>()->value());
}

void CMakeBuildSystem::setAdditionalCMakeArguments(const QStringList &args)
{
    const QStringList expandedAdditionalArguments = Utils::transform(args, [this](const QString &s) {
        return buildConfiguration()->macroExpander()->expand(s);
    });
    const QStringList nonEmptyAdditionalArguments = Utils::filtered(expandedAdditionalArguments,
                                                                    [](const QString &s) {
                                                                        return !s.isEmpty();
                                                                    });
    buildConfiguration()->aspect<AdditionalCMakeOptionsAspect>()->setValue(
        ProcessArgs::joinArgs(nonEmptyAdditionalArguments));
}

void CMakeBuildSystem::filterConfigArgumentsFromAdditionalCMakeArguments()
{
    // On iOS the %{Ios:DevelopmentTeam:Flag} evalues to something like
    // -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM:STRING=MAGICSTRING
    // which is already part of the CMake variables and should not be also
    // in the addtional CMake options
    const QStringList arguments = ProcessArgs::splitArgs(
        buildConfiguration()->aspect<AdditionalCMakeOptionsAspect>()->value());
    QStringList unknownOptions;
    const CMakeConfig config = CMakeConfig::fromArguments(arguments, unknownOptions);

    buildConfiguration()->aspect<AdditionalCMakeOptionsAspect>()->setValue(ProcessArgs::joinArgs(unknownOptions));
}

void CMakeBuildSystem::setError(const QString &message)
{
    qCDebug(cmakeBuildConfigurationLog) << "Setting error to" << message;
    QTC_ASSERT(!message.isEmpty(), return );

    const QString oldMessage = m_error;
    if (m_error != message)
        m_error = message;
    if (oldMessage.isEmpty() != !message.isEmpty()) {
        qCDebug(cmakeBuildConfigurationLog) << "Emitting enabledChanged signal";
        emit buildConfiguration()->enabledChanged();
    }
    TaskHub::addTask(BuildSystemTask(Task::TaskType::Error, message));
    emit errorOccurred(m_error);
}

void CMakeBuildSystem::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    TaskHub::addTask(BuildSystemTask(Task::TaskType::Warning, message));
    emit warningOccurred(m_warning);
}

QString CMakeBuildSystem::error() const
{
    return m_error;
}

QString CMakeBuildSystem::warning() const
{
    return m_warning;
}

NamedWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(m_buildSystem);
}

CMakeConfig CMakeBuildConfiguration::signingFlags() const
{
    return {};
}

void CMakeBuildConfiguration::setInitialBuildAndCleanSteps(const ProjectExplorer::Target *target)
{
    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(
        target->kit());

    int buildSteps = 1;
    if (!presetItem.isNull()) {
        const QString presetName = presetItem.expandedValue(target->kit());
        const CMakeProject *project = static_cast<const CMakeProject *>(target->project());

        const auto buildPresets = project->presetsData().buildPresets;
        const int count
            = std::count_if(buildPresets.begin(),
                            buildPresets.end(),
                            [presetName, project](const PresetsDetails::BuildPreset &preset) {
                                bool enabled = true;
                                if (preset.condition)
                                    enabled = CMakePresets::Macros::evaluatePresetCondition(
                                        preset, project->projectDirectory());

                                return preset.configurePreset == presetName
                                       && !preset.hidden.value() && enabled;
                            });
        if (count != 0)
            buildSteps = count;
    }

    for (int i = 0; i < buildSteps; ++i)
        appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);

    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);
}

void CMakeBuildConfiguration::setBuildPresetToBuildSteps(const ProjectExplorer::Target *target)
{
    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(
        target->kit());

    if (presetItem.isNull())
        return;

    const QString presetName = presetItem.expandedValue(target->kit());
    const CMakeProject *project = static_cast<const CMakeProject *>(target->project());

    const auto allBuildPresets = project->presetsData().buildPresets;
    const auto buildPresets = Utils::filtered(
        allBuildPresets, [presetName, project](const PresetsDetails::BuildPreset &preset) {
            bool enabled = true;
            if (preset.condition)
                enabled = CMakePresets::Macros::evaluatePresetCondition(preset,
                                                                        project->projectDirectory());

            return preset.configurePreset == presetName && !preset.hidden.value() && enabled;
        });

    const QList<BuildStep *> buildStepList
        = Utils::filtered(buildSteps()->steps(), [](const BuildStep *bs) {
              return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
          });

    if (buildPresets.size() != buildStepList.size())
        return;

    for (qsizetype i = 0; i < buildStepList.size(); ++i) {
        CMakeBuildStep *cbs = qobject_cast<CMakeBuildStep *>(buildStepList[i]);
        cbs->setBuildPreset(buildPresets[i].name);
        cbs->setUserEnvironmentChanges(
            getEnvironmentItemsFromCMakeBuildPreset(project, target->kit(), buildPresets[i].name));

        if (buildPresets[i].targets) {
            QString targets = buildPresets[i].targets.value().join(" ");

            CMakePresets::Macros::expand(buildPresets[i],
                                         cbs->environment(),
                                         project->projectDirectory(),
                                         targets);

            cbs->setBuildTargets(targets.split(" "));
        }

        QStringList cmakeArguments;
        if (buildPresets[i].jobs)
            cmakeArguments.append(QString("-j %1").arg(buildPresets[i].jobs.value()));
        if (buildPresets[i].verbose && buildPresets[i].verbose.value())
            cmakeArguments.append("--verbose");
        if (buildPresets[i].cleanFirst && buildPresets[i].cleanFirst.value())
            cmakeArguments.append("--clean-first");
        if (!cmakeArguments.isEmpty())
            cbs->setCMakeArguments(cmakeArguments);

        if (buildPresets[i].nativeToolOptions) {
            QString nativeToolOptions = buildPresets[i].nativeToolOptions.value().join(" ");

            CMakePresets::Macros::expand(buildPresets[i],
                                         cbs->environment(),
                                         project->projectDirectory(),
                                         nativeToolOptions);

            cbs->setToolArguments(nativeToolOptions.split(" "));
        }

        if (buildPresets[i].configuration)
            cbs->setConfiguration(buildPresets[i].configuration.value());

        // Leave only the first build step enabled
        if (i > 0)
            cbs->setEnabled(false);
    }
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

        // Skip the default shadow build directories for build types if we have presets
        const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
        if (!presetItem.isNull())
            return result;

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
    if (bt == "profile")
        return BuildTypeProfile;
    return BuildTypeNone;
}

BuildConfiguration::BuildType CMakeBuildConfigurationFactory::cmakeBuildTypeToBuildType(
    const CMakeBuildConfigurationFactory::BuildType &in)
{
    return createBuildInfo(in).buildType;
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
    case BuildTypeDebug: {
        info.typeName = "Debug";
        info.displayName = BuildConfiguration::tr("Debug");
        info.buildType = BuildConfiguration::Debug;
        QVariantMap extraInfo;
        // enable QML debugging by default
        extraInfo.insert(Constants::QML_DEBUG_SETTING, TriState::Enabled.toVariant());
        info.extraInfo = extraInfo;
        break;
    }
    case BuildTypeRelease:
        info.typeName = "Release";
        info.displayName = BuildConfiguration::tr("Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeMinSizeRel:
        info.typeName = "MinSizeRel";
        info.displayName = Tr::tr("Minimum Size Release");
        info.buildType = BuildConfiguration::Release;
        break;
    case BuildTypeRelWithDebInfo:
        info.typeName = "RelWithDebInfo";
        info.displayName = Tr::tr("Release with Debug Information");
        info.buildType = BuildConfiguration::Profile;
        break;
    case BuildTypeProfile: {
        info.typeName = "Profile";
        info.displayName = Tr::tr("Profile");
        info.buildType = BuildConfiguration::Profile;
        QVariantMap extraInfo;
        // override CMake build type, which defaults to info.typeName
        extraInfo.insert(CMAKE_BUILD_TYPE, "RelWithDebInfo");
        // enable QML debugging by default
        extraInfo.insert(Constants::QML_DEBUG_SETTING, TriState::Enabled.toVariant());
        info.extraInfo = extraInfo;
        break;
    }
    default:
        QTC_CHECK(false);
        break;
    }

    return info;
}

BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    return m_buildSystem->buildType();
}

BuildConfiguration::BuildType CMakeBuildSystem::buildType() const
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

void CMakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    const CMakeTool *tool = CMakeKitAspect::cmakeTool(kit());
    // The hack further down is only relevant for desktop
    if (tool && tool->cmakeExecutable().needsDevice())
        return;

    CMakeSpecificSettings *settings = CMakeProjectPlugin::projectTypeSpecificSettings();
    if (!settings->ninjaPath.filePath().isEmpty()) {
        const Utils::FilePath ninja = settings->ninjaPath.filePath();
        env.appendOrSetPath(ninja.isFile() ? ninja.parentDir() : ninja);
    }
}

Environment CMakeBuildConfiguration::configureEnvironment() const
{
    return d->m_configureEnvironment;
}

void CMakeBuildConfiguration::setUserConfigureEnvironmentChanges(const Utils::EnvironmentItems &diff)
{
    if (d->m_userConfigureEnvironmentChanges == diff)
        return;
    d->m_userConfigureEnvironmentChanges = diff;
    updateAndEmitConfigureEnvironmentChanged();
}

EnvironmentItems CMakeBuildConfiguration::userConfigureEnvironmentChanges() const
{
    return d->m_userConfigureEnvironmentChanges;
}

bool CMakeBuildConfiguration::useClearConfigureEnvironment() const
{
    return d->m_clearSystemConfigureEnvironment;
}

void CMakeBuildConfiguration::setUseClearConfigureEnvironment(bool b)
{
    if (useClearConfigureEnvironment() == b)
        return;
    d->m_clearSystemConfigureEnvironment = b;
    updateAndEmitConfigureEnvironmentChanged();
}

void CMakeBuildConfiguration::updateAndEmitConfigureEnvironmentChanged()
{
    Environment env = baseConfigureEnvironment();
    env.modify(userConfigureEnvironmentChanges());
    if (env == d->m_configureEnvironment)
        return;
    d->m_configureEnvironment = env;
    emit configureEnvironmentChanged();
}

Environment CMakeBuildConfiguration::baseConfigureEnvironment() const
{
    Environment result;
    if (!useClearConfigureEnvironment()) {
        ProjectExplorer::IDevice::ConstPtr devicePtr = BuildDeviceKitAspect::device(kit());
        result = devicePtr ? devicePtr->systemEnvironment() : Environment::systemEnvironment();
    }
    addToEnvironment(result);
    kit()->addToBuildEnvironment(result);
    result.modify(project()->additionalEnvironment());
    return result;
}

QString CMakeBuildConfiguration::baseConfigureEnvironmentText() const
{
    if (useClearConfigureEnvironment())
        return Tr::tr("Clean Environment");
    else
        return Tr::tr("System Environment");
}

QString CMakeBuildSystem::cmakeBuildType() const
{
    auto setBuildTypeFromConfig = [this](const CMakeConfig &config) {
        auto it = std::find_if(config.begin(), config.end(), [](const CMakeConfigItem &item) {
            return item.key == "CMAKE_BUILD_TYPE" && !item.isInitial;
        });
        if (it != config.end())
            const_cast<CMakeBuildSystem*>(this)
                ->setCMakeBuildType(QString::fromUtf8(it->value));
    };

    if (!isMultiConfig())
        setBuildTypeFromConfig(configurationChanges());

    QString cmakeBuildType = buildConfiguration()->aspect<BuildTypeAspect>()->value();

    const Utils::FilePath cmakeCacheTxt = buildConfiguration()->buildDirectory().pathAppended("CMakeCache.txt");
    const bool hasCMakeCache = cmakeCacheTxt.exists();
    CMakeConfig config;

    if (cmakeBuildType == "Unknown") {
        // The "Unknown" type is the case of loading of an existing project
        // that doesn't have the "CMake.Build.Type" aspect saved
        if (hasCMakeCache) {
            QString errorMessage;
            config = CMakeConfig::fromFile(cmakeCacheTxt, &errorMessage);
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

void CMakeBuildSystem::setCMakeBuildType(const QString &cmakeBuildType, bool quiet)
{
    auto aspect = buildConfiguration()->aspect<BuildTypeAspect>();
    if (quiet) {
        aspect->setValueQuietly(cmakeBuildType);
        aspect->update();
    } else {
        aspect->setValue(cmakeBuildType);
    }
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
    QString cmakeGenerator;
    for (QString &arg: arguments) {
        if (arg.startsWith("-G")) {
            const QString strDash(" - ");
            const int idxDash = arg.indexOf(strDash);
            if (idxDash > 0) {
                // -GCodeBlocks - Ninja
                cmakeGenerator = "-DCMAKE_GENERATOR:STRING=" + arg.mid(idxDash + strDash.length());

                arg = arg.left(idxDash);
                arg.replace("-G", "-DCMAKE_EXTRA_GENERATOR:STRING=");

            } else {
                // -GNinja
                arg.replace("-G", "-DCMAKE_GENERATOR:STRING=");
            }
        }
        if (arg.startsWith("-A"))
            arg.replace("-A", "-DCMAKE_GENERATOR_PLATFORM:STRING=");
        if (arg.startsWith("-T"))
            arg.replace("-T", "-DCMAKE_GENERATOR_TOOLSET:STRING=");
    }
    if (!cmakeGenerator.isEmpty())
        arguments.append(cmakeGenerator);

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
    setLabelText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
    setDisplayStyle(LineEditDisplay);
}

// ----------------------------------------------------------------------
// - AdditionalCMakeOptionsAspect:
// ----------------------------------------------------------------------

AdditionalCMakeOptionsAspect::AdditionalCMakeOptionsAspect()
{
    setSettingsKey("CMake.Additional.Options");
    setLabelText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
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
    setSettingsKey(CMAKE_BUILD_TYPE);
    setLabelText(Tr::tr("Build type:"));
    setDisplayStyle(LineEditDisplay);
    setDefaultValue("Unknown");
}

} // namespace Internal
} // namespace CMakeProjectManager
