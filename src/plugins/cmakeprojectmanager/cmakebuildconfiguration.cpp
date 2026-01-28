// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakebuildconfiguration.h"

#include "cmakebuildstep.h"
#include "cmakebuildsystem.h"
#include "cmakeconfigitem.h"
#include "cmakekitaspect.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
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

#include <coreplugin/fileutils.h>
#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/environmentaspectwidget.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/sysrootkitaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>
#include <qtsupport/qtkitaspect.h>

#include <utils/algorithm.h>
#include <utils/categorysortfiltermodel.h>
#include <utils/checkablemessagebox.h>
#include <utils/commandline.h>
#include <utils/detailswidget.h>
#include <utils/infolabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHeaderView>
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

const char DEVELOPMENT_TEAM_FLAG[] = "Ios:DevelopmentTeam:Flag";
const char PROVISIONING_PROFILE_FLAG[] = "Ios:ProvisioningProfile:Flag";
const char CMAKE_OSX_ARCHITECTURES_FLAG[] = "CMAKE_OSX_ARCHITECTURES:DefaultFlag";
const char QT_QML_DEBUG_FLAG[] = "Qt:QML_DEBUG_FLAG";
const char QT_QML_DEBUG_PARAM[] = "-DQT_QML_DEBUG";
const char CMAKE_QT6_TOOLCHAIN_FILE_ARG[]
    = "-DCMAKE_TOOLCHAIN_FILE:FILEPATH=%{Qt:QT_INSTALL_PREFIX}/lib/cmake/Qt6/qt.toolchain.cmake";
const char CMAKE_BUILD_TYPE[] = "CMake.Build.Type";
const char CLEAR_SYSTEM_ENVIRONMENT_KEY[] = "CMake.Configure.ClearSystemEnvironment";
const char USER_ENVIRONMENT_CHANGES_KEY[] = "CMake.Configure.UserEnvironmentChanges";
const char BASE_ENVIRONMENT_KEY[] = "CMake.Configure.BaseEnvironment";

const char CMAKE_TOOLCHAIN_FILE[] = "CMAKE_TOOLCHAIN_FILE";
const char CMAKE_C_FLAGS_INIT[] = "CMAKE_C_FLAGS_INIT";
const char CMAKE_CXX_FLAGS_INIT[] = "CMAKE_CXX_FLAGS_INIT";
const char CMAKE_CXX_FLAGS[] = "CMAKE_CXX_FLAGS";
const char CMAKE_CXX_FLAGS_DEBUG[] = "CMAKE_CXX_FLAGS_DEBUG";
const char CMAKE_CXX_FLAGS_RELWITHDEBINFO[] = "CMAKE_CXX_FLAGS_RELWITHDEBINFO";
const char QT_CREATOR_ENABLE_PACKAGE_MANAGER_SETUP[] = "QT_CREATOR_ENABLE_PACKAGE_MANAGER_SETUP";
const char QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER[] = "QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER";
const char QT_ENABLE_QML_DEBUG_FLAG[] = "Qt:QT_ENABLE_QML_DEBUG";
const char QT_ENABLE_QML_DEBUG[] = "QT_ENABLE_QML_DEBUG";

const char CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM[] = "CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM";
const char CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[]
    = "CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER";

namespace Internal {

struct AspectToCMakeConfigItem
{
    BoolAspect &aspect;
    CMakeConfigItem configItem;
};
using AspectToConfigItemList = QList<AspectToCMakeConfigItem>;

class CMakeBuildSettingsWidget : public QWidget
{
public:
    explicit CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc);
    ~CMakeBuildSettingsWidget();

    void setError(const QString &message);
    void setWarning(const QString &message);

    void updateInitialCMakeArguments(bool fromReconfigure = false);

private:
    void updateButtonState();
    void updateAdvancedCheckBox();
    void updateFromKit();
    void updateConfigurationStateIndex(int index);
    CMakeConfig getQmlDebugConfigItem();
    CMakeConfig getSigningFlagsChanges();
    CMakeConfig getSettingsToCMakeConfigItems();

    void updateSelection();
    void updateConfigurationStateSelection();
    bool isInitialConfiguration() const;
    void setVariableUnsetFlag(bool unsetFlag);
    QAction *createForceAction(int type, const QModelIndex &idx);

    bool eventFilter(QObject *target, QEvent *event) override;

    void batchEditConfiguration();
    void reconfigureWithInitialParameters();
    void kitCMakeConfiguration();
    void updateConfigureDetailsWidgetsSummary(
        const QStringList &configurationArguments = QStringList());
    void updateSettingsToCMakeConfigItems(CMakeConfig &initialList);

    AspectToConfigItemList getAspectToConfigureItemList();

    QPointer<CMakeBuildConfiguration> m_buildConfig;
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
    CMakeConfig m_configurationChanges;
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

static CMakeConfigItem getCMakeHelperParameter()
{
    const QByteArray key("CMAKE_PROJECT_INCLUDE_BEFORE");
    const QByteArray value = QString(
                                 "%{BuildConfig:BuildDirectory:NativeFilePath}/%1/qtcreator-project.cmake")
                                 .arg(Constants::PACKAGE_MANAGER_DIR)
                                 .toUtf8();
    return CMakeConfigItem(key, CMakeConfigItem::FILEPATH, value);
}

static CMakeConfigItem enablePackageManagerAutoSetupParameter()
{
    return CMakeConfigItem(QT_CREATOR_ENABLE_PACKAGE_MANAGER_SETUP, CMakeConfigItem::BOOL, "ON");
}

static CMakeConfigItem enableMaintenanceToolDependencyProvider()
{
    return CMakeConfigItem(QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER, CMakeConfigItem::BOOL, "ON");
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeBuildConfiguration *bc) :
    m_buildConfig(bc),
    m_configModel(new ConfigModel(this)),
    m_configFilterModel(new CategorySortFilterModel(this)),
    m_configTextFilterModel(new CategorySortFilterModel(this))
{
    m_configureDetailsWidget = new DetailsWidget;

    updateConfigureDetailsWidgetsSummary();

    auto details = new QWidget(m_configureDetailsWidget);
    m_configureDetailsWidget->setWidget(details);

    auto buildDirAspect = bc->buildDirectoryAspect();
    buildDirAspect->setAutoApplyOnEditingFinished(true);
    buildDirAspect->addOnChanged(this, [this] { m_configModel->flush(); }); // clear config cache

    m_buildConfig->buildTypeAspect.addOnChanged(this, [this] {
        if (!m_buildConfig->cmakeBuildSystem()->isMultiConfig()) {
            CMakeConfig config;
            config.insert(
                CMakeConfigItem("CMAKE_BUILD_TYPE", m_buildConfig->buildTypeAspect().toUtf8()));

            m_configModel->setBatchEditConfiguration(config);
        }
    });

    auto qmlDebugAspect = bc->aspect<QtSupport::QmlDebuggingAspect>();
    qmlDebugAspect->addOnChanged(this, [this] { updateButtonState(); });

    if (CMakeProject *cmp = qobject_cast<CMakeProject *>(bc->project())) {
        cmp->settings().packageManagerAutoSetup.addOnChanged(this, [this] {
            if (m_buildConfig) {
                updateButtonState();
                m_buildConfig->cmakeBuildSystem()->runCMakeWithExtraArguments();
            }
        });
        cmp->settings().maintenanceToolDependencyProvider.addOnChanged(this, [this] {
            if (m_buildConfig) {
                updateButtonState();
                m_buildConfig->cmakeBuildSystem()->runCMakeWithExtraArguments();
            }
        });
    }

    m_warningMessageLabel = new InfoLabel({}, InfoLabel::Warning);
    m_warningMessageLabel->setVisible(false);

    m_configurationStates = new QTabBar(this);
    m_configurationStates->addTab(Tr::tr("Initial Configuration"));
    m_configurationStates->addTab(Tr::tr("Current Configuration"));
    setWheelScrollingWithoutFocusBlocked(m_configurationStates);
    connect(m_configurationStates, &QTabBar::currentChanged, this, [this](int index) {
        updateConfigurationStateIndex(index);
    });

    m_kitConfiguration = new QPushButton(Tr::tr("Kit Configuration"));
    m_kitConfiguration->setToolTip(Tr::tr("Edit the current kit's CMake configuration."));
    m_kitConfiguration->setFixedWidth(m_kitConfiguration->sizeHint().width());
    connect(m_kitConfiguration, &QPushButton::clicked,
            this, &CMakeBuildSettingsWidget::kitCMakeConfiguration,
            Qt::QueuedConnection);

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
    m_configTextFilterModel->setNewItemRole(ConfigModel::ItemIsUserNew);

    connect(m_configTextFilterModel, &QAbstractItemModel::layoutChanged, this, [this] {
        QModelIndex selectedIdx = m_configView->currentIndex();
        if (selectedIdx.isValid())
            m_configView->scrollTo(selectedIdx);
    });

    m_configView->setModel(m_configTextFilterModel);
    m_configView->setMinimumHeight(300);
    m_configView->setSortingEnabled(true);
    m_configView->sortByColumn(0, Qt::AscendingOrder);
    m_configView->header()->setSectionResizeMode(QHeaderView::Stretch);
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
    connect(&m_showProgressTimer, &QTimer::timeout, this, [this] { m_progressIndicator->show(); });

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
    m_showAdvancedCheckBox->setChecked(
        settings(m_buildConfig->project()).showAdvancedOptionsByDefault());

    connect(m_configView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this](const QItemSelection &, const QItemSelection &) {
                updateSelection();
    });

    m_reconfigureButton = new QPushButton(Tr::tr("Run CMake"));
    m_reconfigureButton->setEnabled(false);

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

    auto configureEnvironmentAspectWidget = bc->configureEnv.createConfigWidget();
    configureEnvironmentAspectWidget->setContentsMargins(0, 0, 0, 0);
    configureEnvironmentAspectWidget->layout()->setContentsMargins(0, 0, 0, 0);

    Column {
        Form {
            buildDirAspect, br,
            bc->buildTypeAspect, br,
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
                        bc->initialCMakeArguments,
                        bc->additionalCMakeOptions
                    },
                    m_reconfigureButton,
                }
            },
            configureEnvironmentAspectWidget
        },
        noMargin
    }.attachTo(details);

    Column {
        m_configureDetailsWidget,
        noMargin
    }.attachTo(this);

    updateAdvancedCheckBox();

    CMakeBuildSystem *bs = m_buildConfig->cmakeBuildSystem();
    setError(bs->error());
    setWarning(bs->warning());

    connect(bs, &BuildSystem::parsingStarted, this, [this] {
        updateButtonState();
        m_configView->setEnabled(false);
        m_showProgressTimer.start();
    });

    m_configModel->setMacroExpander(bc->macroExpander());

    if (bs->isParsing())
        m_showProgressTimer.start();
    else {
        m_configModel->setConfiguration(bs->configurationFromCMake());
        m_configModel->setInitialParametersConfiguration(
            m_buildConfig->initialCMakeArguments.cmakeConfiguration());
    }

    connect(bs, &BuildSystem::parsingFinished, this, [this] {
        QTC_ASSERT(m_buildConfig, return);
        CMakeBuildSystem *bs = m_buildConfig->cmakeBuildSystem();
        QTC_ASSERT(bs, return);
        if (bs->isDestructing())
            return;
        const CMakeConfig config = bs->configurationFromCMake();
        const TriState qmlDebugSetting = m_buildConfig->qmlDebugging();
        bool qmlDebugConfig = CMakeBuildConfiguration::hasQmlDebugging(config);
        if ((qmlDebugSetting == TriState::Enabled && !qmlDebugConfig)
            || (qmlDebugSetting == TriState::Disabled && qmlDebugConfig)) {
            m_buildConfig->qmlDebugging.setValue(TriState::Default);
        }
        m_configModel->setConfiguration(config);
        m_configModel->setInitialParametersConfiguration(
            CMakeBuildConfiguration::updateCMakeHelperConfig(
                m_buildConfig->initialCMakeArguments.cmakeConfiguration()));
        m_buildConfig->filterConfigArgumentsFromAdditionalCMakeArguments();
        updateFromKit();
        m_configView->setEnabled(true);
        updateButtonState();
        m_showProgressTimer.stop();
        m_progressIndicator->hide();

        if (!m_configurationChanges.isEmpty()) {
            m_configModel->setBatchEditConfiguration(m_configurationChanges);
            m_configurationChanges.clear();
        }
        updateConfigurationStateSelection();
    });

    connect(bs, &CMakeBuildSystem::configurationCleared, this, [this] {
        updateConfigurationStateSelection();
    });

    connect(bs, &CMakeBuildSystem::errorOccurred, this, [this] {
        m_showProgressTimer.stop();
        m_progressIndicator->hide();
        updateConfigurationStateSelection();
    });

    connect(m_configModel, &QAbstractItemModel::dataChanged,
            this, &CMakeBuildSettingsWidget::updateButtonState);
    connect(m_configModel, &QAbstractItemModel::modelReset,
            this, &CMakeBuildSettingsWidget::updateButtonState);

    connect(m_buildConfig, &CMakeBuildConfiguration::signingFlagsChanged,
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

    connect(m_resetButton, &QPushButton::clicked, this, [this] {
        m_configModel->resetAllChanges(isInitialConfiguration());
    });
    connect(m_reconfigureButton, &QPushButton::clicked, this, [this, bs] {
        if (!bs->isParsing()) {
            if (isInitialConfiguration()) {
                reconfigureWithInitialParameters();
            } else {
                bs->runCMakeWithExtraArguments();
            }
        } else {
            bs->stopCMakeRun();
            m_reconfigureButton->setEnabled(false);
        }
    });
    connect(m_setButton, &QPushButton::clicked, this, [this] { setVariableUnsetFlag(false); });
    connect(m_unsetButton, &QPushButton::clicked, this, [this] {
        setVariableUnsetFlag(true);
    });
    connect(m_editButton, &QPushButton::clicked, this, [this] {
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

    connect(bs, &CMakeBuildSystem::errorOccurred,
            this, &CMakeBuildSettingsWidget::setError);
    connect(bs, &CMakeBuildSystem::warningOccurred,
            this, &CMakeBuildSettingsWidget::setWarning);

    connect(bs, &CMakeBuildSystem::configurationChanged, this, [this](const CMakeConfig &config) {
        m_configurationChanges = config;
    });

    updateFromKit();
    connect(m_buildConfig, &BuildConfiguration::kitChanged,
            this, &CMakeBuildSettingsWidget::updateFromKit);
    connect(bc, &CMakeBuildConfiguration::enabledChanged, this, [this, bc] {
        if (bc->isEnabled())
            setError(QString());
    });
    connect(m_buildConfig->project(), &Project::aboutToSaveSettings, this, [this] {
        updateInitialCMakeArguments();
    });

    connect(&bc->initialCMakeArguments,
            &Utils::BaseAspect::labelLinkActivated,
            this,
            [this](const QString &) {
                CMakeKitAspect::openCMakeHelpUrl(m_buildConfig->kit(), "%1/manual/cmake.1.html#options");
            });
    connect(&bc->additionalCMakeOptions,
            &Utils::BaseAspect::labelLinkActivated, this, [this](const QString &) {
                CMakeKitAspect::openCMakeHelpUrl(m_buildConfig->kit(), "%1/manual/cmake.1.html#options");
            });

    if (HostOsInfo::isMacHost())
        m_configurationStates->setDrawBase(false);
    m_configurationStates->setExpanding(false);
    m_reconfigureButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    updateSelection();
    updateConfigurationStateSelection();
}

CMakeBuildSettingsWidget::~CMakeBuildSettingsWidget()
{
    updateInitialCMakeArguments();
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
        CMakeKitAspect::openCMakeHelpUrl(m_buildConfig->kit(), "%1/manual/cmake-variables.7.html");
    });
    editor->setMinimumSize(800, 200);

    auto chooser = new Utils::VariableChooser(dialog);
    chooser->addSupportedWidget(editor);
    chooser->addMacroExpanderProvider({this, [this] { return m_buildConfig->macroExpander(); }});

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);

    layout->addWidget(editor);
    layout->addWidget(label);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::accepted, this, [this, editor] {
        const auto expander = m_buildConfig->macroExpander();

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
        m_buildConfig->cmakeBuildSystem()->configurationChangesArguments(isInitialConfiguration())
            .join('\n'));

    dialog->show();
}

void CMakeBuildSettingsWidget::reconfigureWithInitialParameters()
{
    QMessageBox::StandardButton reply = CheckableMessageBox::question(
        Tr::tr("Re-configure with Initial Parameters"),
        Tr::tr("Clear CMake configuration and configure with initial parameters?"),
        settings(m_buildConfig->project()).askBeforeReConfigureInitialParams.askAgainCheckableDecider(),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    settings(m_buildConfig->project()).writeSettings();

    if (reply != QMessageBox::Yes)
        return;

    updateInitialCMakeArguments(true);

    m_buildConfig->cmakeBuildSystem()->clearCMakeCache();

    if (ProjectExplorerPlugin::saveModifiedFiles())
        m_buildConfig->cmakeBuildSystem()->runCMake();
}

static bool haveValidCMakeProjectIncludeBefore(const CMakeConfig &configList)
{
    const FilePath cmakeProjectIncludeBefore = configList.filePathValueOf(
        "CMAKE_PROJECT_INCLUDE_BEFORE");
    if (cmakeProjectIncludeBefore.isEmpty()
        || cmakeProjectIncludeBefore.fileName() != "qtcreator-project.cmake")
        // These settings only work if cmake-helper/qtcreator-project.cmake is included
        return false;

    return true;
}

void CMakeBuildSettingsWidget::updateSettingsToCMakeConfigItems(CMakeConfig &initialList)
{
    AspectToConfigItemList list = getAspectToConfigureItemList();
    if (!haveValidCMakeProjectIncludeBefore(initialList))
        return;

    for (auto &a2c : list) {
        auto it
            = std::find_if(initialList.begin(), initialList.end(), [a2c](const CMakeConfigItem &item) {
                  return item.key == a2c.configItem.key;
              });
        if (it != initialList.end()) {
            if (!a2c.aspect() && it->value == a2c.configItem.value)
                initialList.erase(it);
        } else if (a2c.aspect()) {
            initialList.insert(a2c.configItem);
        }
    }
}

AspectToConfigItemList CMakeBuildSettingsWidget::getAspectToConfigureItemList()
{
    AspectToConfigItemList list
        = {{settings(m_buildConfig->project()).packageManagerAutoSetup,
            enablePackageManagerAutoSetupParameter()},
           {settings(m_buildConfig->project()).maintenanceToolDependencyProvider,
            enableMaintenanceToolDependencyProvider()}};
    return list;
}

static bool isGenerateQmllsSettingsEnabled()
{
    constexpr char settingsKey[] = "LanguageClient/typedClients";
    constexpr char qmllsTypeId[] = "LanguageClient::QmllsClientSettingsID";
    constexpr char typeIdKey[] = "typeId";
    constexpr char generateQmllsIniFilesKey[] = "generateQmllsIniFiles";

    const QtcSettings *settings = Core::ICore::settings();
    for (const QVariant &client : settings->value(settingsKey).toList()) {
        const Store map = storeFromVariant(client);
        if (map.value(typeIdKey).toString() == qmllsTypeId)
            return map[generateQmllsIniFilesKey].toBool();
    }
    QTC_ASSERT(false, return false);
}

void CMakeBuildSettingsWidget::updateInitialCMakeArguments(bool fromReconfigure)
{
    QTC_ASSERT(m_buildConfig, return);
    QTC_ASSERT(m_buildConfig->cmakeBuildSystem(), return);

    CMakeConfig initialList = m_buildConfig->initialCMakeArguments.cmakeConfiguration();

    // set QT_QML_GENERATE_QMLLS_INI if it is enabled via the settings checkbox and if its not part
    // of the initial CMake arguments yet
    if (isGenerateQmllsSettingsEnabled() && !initialList.contains("QT_QML_GENERATE_QMLLS_INI")) {
        const QtSupport::QtVersion *qtVersion = QtSupport::QtKitAspect::qtVersion(
            m_buildConfig->kit());
        if (qtVersion && qtVersion->qtVersion() < QVersionNumber(6, 10, 0)) {
            initialList.insert(
                CMakeConfigItem("QT_QML_GENERATE_QMLLS_INI", CMakeConfigItem::BOOL, "ON"));
        }
    }

    for (const CMakeConfigItem &ci : m_buildConfig->cmakeBuildSystem()->configurationChanges()) {
        if (!ci.isInitial)
            continue;
        if (initialList.contains(ci.key)) {
            initialList[ci.key] = ci;
            if (ci.isUnset)
                initialList.remove(ci.key);
        } else if (!ci.key.isEmpty()) {
            initialList.insert(ci);
        }
    }

    updateSettingsToCMakeConfigItems(initialList);

    m_buildConfig->initialCMakeArguments.setCMakeConfiguration(
        CMakeBuildConfiguration::updateCMakeHelperConfig(initialList));

    if (fromReconfigure) {
        // value() will contain only the unknown arguments (the non -D/-U arguments)
        // As the user would expect to have e.g. "--preset" from "Initial Configuration"
        // to "Current Configuration" as additional parameters
        m_buildConfig->setAdditionalCMakeArguments(ProcessArgs::splitArgs(
            m_buildConfig->initialCMakeArguments(), HostOsInfo::hostOs()));
    }
}

void CMakeBuildSettingsWidget::kitCMakeConfiguration()
{
    using namespace Layouting;
    m_buildConfig->kit()->blockNotification();

    auto dialog = new QDialog(this);
    auto deleteLater = [dialog](BaseAspect *aspect) -> BaseAspect * {
        aspect->setParent(dialog);
        return aspect;
    };

    dialog->setWindowTitle(Tr::tr("Kit CMake Configuration"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->setSizeGripEnabled(true);
    connect(dialog, &QDialog::finished, this, [this] {
        m_buildConfig->kit()->unblockNotification();
    });

    Kit *kit = m_buildConfig->kit();

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::clicked, dialog, &QDialog::close);

    Grid {
        deleteLater(CMakeKitAspect::createKitAspect(kit)),
        deleteLater(CMakeGeneratorKitAspect::createKitAspect(kit)),
        deleteLater(CMakeConfigurationKitAspect::createKitAspect(kit)),
        empty, empty, buttons,
        columnStretch(1, 1)
    }.attachTo(dialog);

    dialog->setMinimumWidth(400);
    dialog->resize(800, 1);
    dialog->show();
}

void CMakeBuildSettingsWidget::updateConfigureDetailsWidgetsSummary(
    const QStringList &configurationArguments)
{
    FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(m_buildConfig->kit());
    if (cmakeExecutable.isEmpty())
        cmakeExecutable = "cmake";

    CommandLine cmd;
    cmd.setExecutable(cmakeExecutable);
    cmd.addArgs({"-S", m_buildConfig->project()->projectDirectory().path()});
    cmd.addArgs({"-B", m_buildConfig->buildDirectory().path()});
    cmd.addArgs(configurationArguments);

    ProcessParameters params;
    params.setCommandLine(cmd);

    m_configureDetailsWidget->setSummaryText(params.summary(Tr::tr("Configure")));
    m_configureDetailsWidget->setState(DetailsWidget::Expanded);
}

void CMakeBuildSettingsWidget::setError(const QString &message)
{
    m_buildConfig->buildDirectoryAspect()->setProblem(message);
}

void CMakeBuildSettingsWidget::setWarning(const QString &message)
{
    bool showWarning = !message.isEmpty();
    m_warningMessageLabel->setVisible(showWarning);
    m_warningMessageLabel->setText(message);
}

void CMakeBuildSettingsWidget::updateButtonState()
{
    QTC_ASSERT(m_buildConfig, return);
    const bool isParsing = m_buildConfig->cmakeBuildSystem()->isParsing();

    // Update extra data in buildconfiguration
    const QList<ConfigModel::DataItem> changes = m_configModel->configurationForCMake();

    const CMakeConfig configChanges
        = getQmlDebugConfigItem() + getSigningFlagsChanges() + getSettingsToCMakeConfigItems()
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

    m_buildConfig->initialCMakeArguments.setVisible(isInitialConfiguration());
    m_buildConfig->additionalCMakeOptions.setVisible(!isInitialConfiguration());

    m_buildConfig->initialCMakeArguments.setEnabled(!isParsing);
    m_buildConfig->additionalCMakeOptions.setEnabled(!isParsing);

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

    m_buildConfig->cmakeBuildSystem()->setConfigurationChanges(configChanges);

    // Update the tooltip with the changes
    const QStringList configurationArguments =
        m_buildConfig->cmakeBuildSystem()->configurationChangesArguments(isInitialConfiguration());
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
    const Kit *k = m_buildConfig->kit();
    CMakeConfig config = CMakeConfigurationKitAspect::configuration(k);

    config.insert(CMakeGeneratorKitAspect::generatorCMakeConfig(k));

    // First the key value parameters
    ConfigModel::KitConfiguration configHash;
    for (const CMakeConfigItem &i : std::as_const(config))
        configHash.insert(QString::fromUtf8(i.key), i);

    m_configModel->setConfigurationFromKit(configHash);

    // Then the additional parameters
    const QStringList additionalKitCMake = ProcessArgs::splitArgs(
        CMakeConfigurationKitAspect::additionalConfiguration(k), HostOsInfo::hostOs());
    const QStringList additionalInitialCMake =
        ProcessArgs::splitArgs(m_buildConfig->initialCMakeArguments(), HostOsInfo::hostOs());

    QStringList mergedArgumentList;
    std::set_union(additionalInitialCMake.begin(),
                   additionalInitialCMake.end(),
                   additionalKitCMake.begin(),
                   additionalKitCMake.end(),
                   std::back_inserter(mergedArgumentList));
    m_buildConfig->initialCMakeArguments.setValue(ProcessArgs::joinArgs(mergedArgumentList));
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

static CMakeConfig removeOldQmlConfigSettings(const CMakeConfig &configList)
{
    // Remove any Qt Creator 18 and lower settings
    CMakeConfig changedConfig;

    const QByteArrayList cxxFlagsPrev{
        CMAKE_CXX_FLAGS,
        CMAKE_CXX_FLAGS_DEBUG,
        CMAKE_CXX_FLAGS_RELWITHDEBINFO,
        CMAKE_CXX_FLAGS_INIT};
    const QByteArray qmlDebug(QT_QML_DEBUG_PARAM);

    for (const CMakeConfigItem &item : configList) {
        if (!cxxFlagsPrev.contains(item.key))
            continue;

        CMakeConfigItem it(item);
        int index = it.value.indexOf(qmlDebug);
        if (index != -1) {
            it.value.remove(index, qmlDebug.length());
            it.value = it.value.trimmed();
            changedConfig.insert(it);
        }
    }
    return changedConfig;
}

CMakeConfig CMakeBuildSettingsWidget::getQmlDebugConfigItem()
{
    const bool enable = m_buildConfig->qmlDebugging() == TriState::Enabled;

    const CMakeConfig configList = m_buildConfig->cmakeBuildSystem()->configurationFromCMake();
    CMakeConfig changedConfig;
    if (configList.isEmpty())
        // we don't have any configuration --> initial configuration takes care of this itself
        return {};

    CMakeConfigItem
        qmlDebugItem(QT_ENABLE_QML_DEBUG, CMakeConfigItem::BOOL, "", enable ? "ON" : "OFF");

    const bool haveQtQmlDebug = CMakeBuildConfiguration::hasQmlDebugging(configList);
    if (enable) {
        if (!haveQtQmlDebug)
            changedConfig.insert(qmlDebugItem);
    } else if (haveQtQmlDebug) {
        qmlDebugItem.isUnset = true;
        changedConfig.insert(qmlDebugItem);

        changedConfig.insert(removeOldQmlConfigSettings(configList));
    }
    return changedConfig;
}

CMakeConfig CMakeBuildSettingsWidget::getSigningFlagsChanges()
{
    const CMakeConfig flags = m_buildConfig->signingFlags();
    if (flags.isEmpty())
        return {};
    const CMakeConfig configList = m_buildConfig->cmakeBuildSystem()->configurationFromCMake();
    if (configList.isEmpty()) {
        // we don't have any configuration --> initial configuration takes care of this itself
        return {};
    }
    CMakeConfig changedConfig;
    for (const CMakeConfigItem &signingFlag : flags) {
        const CMakeConfigItem existingFlag = configList.value(signingFlag.key);
        const bool notInConfig = existingFlag.key.isEmpty();
        if (notInConfig != signingFlag.isUnset || existingFlag.value != signingFlag.value)
            changedConfig.insert(signingFlag);
    }
    return changedConfig;
}

CMakeConfig CMakeBuildSettingsWidget::getSettingsToCMakeConfigItems()
{
    AspectToConfigItemList aspectsToConfigs = getAspectToConfigureItemList();

    const CMakeConfig configList = m_buildConfig->cmakeBuildSystem()->configurationFromCMake();
    CMakeConfig changedConfig;
    if (configList.isEmpty())
        // we don't have any configuration --> initial configuration takes care of this itself
        return {};

    if (m_buildConfig->extraData(Constants::CMAKE_IMPORTED_BUILD).toBool())
        // Don't touch the imported builds
        return {};

    if (!haveValidCMakeProjectIncludeBefore(configList))
        return {};

    for (auto &a2c : aspectsToConfigs) {
        a2c.configItem.value = a2c.aspect() ? "ON" : "OFF";
        bool hasExistingValue = configList.contains(a2c.configItem.key);

        if (a2c.aspect()) {
            if (!hasExistingValue)
                changedConfig.insert(a2c.configItem);
        } else if (hasExistingValue) {
            a2c.configItem.isUnset = true;
            changedConfig.insert(a2c.configItem);
        }
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
        = FileApiParser::scanForCMakeReplyFile(m_buildConfig->buildDirectory()).exists();

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
            this, [this, idx, t] { m_configModel->forceTo(idx, t); });
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
    connect(help, &QAction::triggered, this, [this, idx] {
        const CMakeConfigItem item = ConfigModel::dataItemFromIndex(idx).toCMakeConfigItem();
        const QString linkUrl = "%1/variable/" + QString::fromUtf8(item.key) + ".html";
        CMakeKitAspect::openCMakeHelpUrl(m_buildConfig->kit(), linkUrl);
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
                  const QString value
                      = ConfigModel::dataItemFromIndex(index).toCMakeConfigItem().toArgument(
                          isInitialConfiguration() ? nullptr : m_buildConfig->macroExpander());

                  return value.contains(QChar::Space) ? QString(R"("%1")").arg(value) : value;
              });

        setClipboardAndSelection(variableList.join('\n'));
    });

    menu->move(e->globalPos());
    menu->show();

    return true;
}

static bool isWebAssembly(const Kit *k)
{
    return RunDeviceTypeKitAspect::deviceTypeId(k) == WebAssembly::Constants::WEBASSEMBLY_DEVICE_TYPE;
}

static bool isVxWorks(const Kit *k)
{
    return RunDeviceTypeKitAspect::deviceTypeId(k) == Constants::VXWORKS_DEVICE_TYPE;
}

static bool isQnx(const Kit *k)
{
    return RunDeviceTypeKitAspect::deviceTypeId(k) == Qnx::Constants::QNX_QNX_OS_TYPE;
}

static bool isWindowsARM64(const Kit *k)
{
    Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(k);
    if (!toolchain)
        return false;
    const Abi targetAbi = toolchain->targetAbi();
    return targetAbi.os() == Abi::WindowsOS && targetAbi.architecture() == Abi::ArmArchitecture
           && targetAbi.wordWidth() == 64;
}

static CommandLine defaultInitialCMakeCommand(
    const Kit *k, Project *project, const QString &buildType)
{
    // Generator:
    const FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(k);
    QTC_ASSERT(!cmakeExecutable.isEmpty(), return {});

    CommandLine cmd{cmakeExecutable};
    cmd.addArgs(CMakeGeneratorKitAspect::generatorCMakeConfig(k).toArguments());

    // CMAKE_BUILD_TYPE:
    if (!buildType.isEmpty() && !CMakeGeneratorKitAspect::isMultiConfigGenerator(k))
        cmd.addArg("-DCMAKE_BUILD_TYPE:STRING=" + buildType);

    // CMake-helper
    cmd.addArg(getCMakeHelperParameter().toArgument());

    // Package manager auto setup
    if (settings(project).packageManagerAutoSetup())
        cmd.addArg(enablePackageManagerAutoSetupParameter().toArgument());

    if (settings(project).maintenanceToolDependencyProvider())
        cmd.addArg(enableMaintenanceToolDependencyProvider().toArgument());

    // Cross-compilation settings:
    if (!CMakeBuildConfiguration::isIos(k)) { // iOS handles this differently
        const QString sysRoot = SysRootKitAspect::sysRoot(k).path();
        if (!sysRoot.isEmpty()) {
            cmd.addArg("-DCMAKE_SYSROOT:PATH=" + sysRoot);
            if (Toolchain *tc = ToolchainKitAspect::cxxToolchain(k)) {
                const QString targetTriple = tc->originalTargetTriple();
                cmd.addArg("-DCMAKE_C_COMPILER_TARGET:STRING=" + targetTriple);
                cmd.addArg("-DCMAKE_CXX_COMPILER_TARGET:STRING=" + targetTriple);
            }
        }
    }

    // CMake should output colors by default
    cmd.addArg("-DCMAKE_COLOR_DIAGNOSTICS:BOOL=ON");

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

    if (configurePreset.graphviz) {
        QString graphvizValue = configurePreset.graphviz.value();
        CMakePresets::Macros::expand(configurePreset, env, project->projectDirectory(), graphvizValue);
        initialArguments.append("--graphviz=" + graphvizValue);
    }

    if (configurePreset.trace) {
        const auto &trace = configurePreset.trace.value();
        if (trace.mode)
            initialArguments.append("--trace=" + trace.mode.value());
        if (trace.format)
            initialArguments.append("--trace-format=" + trace.format.value());
        if (trace.source) {
            for (const QString &source : trace.source.value())
                initialArguments.append("--trace-source=" + source);
        }
        if (trace.redirect)
            initialArguments.append("--trace-redirect=" + trace.redirect.value());
    }

    CMakePresets::Macros::updateToolchainFile(configurePreset,
                                              env,
                                              project->projectDirectory(),
                                              buildDirectory);
    CMakePresets::Macros::updateInstallDir(configurePreset, env, project->projectDirectory());

    // Merge the presets cache variables
    CMakeConfig cache;
    if (configurePreset.cacheVariables)
        cache = configurePreset.cacheVariables.value();

    for (const CMakeConfigItem &presetItemRaw : std::as_const(cache)) {

        // Expand the CMakePresets Macros
        CMakeConfigItem presetItem(presetItemRaw);

        QString presetItemValue = QString::fromUtf8(presetItem.value);
        CMakePresets::Macros::expand(configurePreset, env, project->projectDirectory(), presetItemValue);
        presetItem.value = presetItemValue.toUtf8();

        const QString presetItemArg = presetItem.toArgument();
        const QString presetItemArgNoType = presetItemArg.left(presetItemArg.indexOf(":"));

        static QSet<QByteArray> defaultKitMacroValues{"CMAKE_C_COMPILER",
                                                      "CMAKE_CXX_COMPILER",
                                                      "QT_QMAKE_EXECUTABLE",
                                                      "QT_HOST_PATH",
                                                      "CMAKE_PROJECT_INCLUDE_BEFORE"};

        auto it = std::find_if(initialArguments.begin(),
                               initialArguments.end(),
                               [presetItemArgNoType](const QString &arg) {
                                   return arg.startsWith(presetItemArgNoType);
                               });

        if (it != initialArguments.end()) {
            QString &arg = *it;
            CMakeConfigItem argItem = CMakeConfigItem::fromString(arg.mid(2)); // skip -D

            // These values have Qt Creator macro names pointing to the Kit values
            // which are preset expanded values used when the Kit was created
            if (defaultKitMacroValues.contains(argItem.key) && argItem.value.startsWith("%{"))
                continue;

            // For multi value path variables append the non Qt path
            if (argItem.key == "CMAKE_PREFIX_PATH" || argItem.key == "CMAKE_FIND_ROOT_PATH") {
                QStringList presetValueList = presetItem.expandedValue(k).split(";");

                // Remove the expanded Qt path from the presets values
                QString argItemExpandedValue = argItem.expandedValue(k);
                presetValueList.removeIf([argItemExpandedValue](const QString &presetPath) {
                    const QStringList argItemPaths = argItemExpandedValue.split(";");
                    for (const QString &argPath : argItemPaths) {
                        const FilePath argFilePath = FilePath::fromString(argPath);
                        const FilePath presetFilePath = FilePath::fromUserInput(presetPath);

                        if (argFilePath == presetFilePath)
                            return true;
                    }
                    return false;
                });

                // Add the presets values to the final argument
                for (const QString &presetPath : std::as_const(presetValueList)) {
                    argItem.value.append(";");
                    argItem.value.append(presetPath.toUtf8());
                }

                arg = argItem.toArgument();
            } else if (argItem.key == CMAKE_TOOLCHAIN_FILE) {
                const FilePath argFilePath = FilePath::fromString(argItem.expandedValue(k));
                const FilePath presetFilePath = FilePath::fromUserInput(
                    QString::fromUtf8(presetItem.value));

                if (argFilePath != presetFilePath)
                    arg = presetItem.toArgument();
            } else if (argItem.key == CMAKE_C_FLAGS_INIT || argItem.key == CMAKE_CXX_FLAGS_INIT) {
                // Append the preset value with at the initial parameters value (e.g. QML Debugging)
                if (argItem.expandedValue(k) != QString::fromUtf8(presetItem.value)) {
                    argItem.value.append(" ");
                    argItem.value.append(presetItem.value);

                    arg = argItem.toArgument();
                }
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

} // namespace Internal

// -----------------------------------------------------------------------------
// CMakeBuildConfiguration:
// -----------------------------------------------------------------------------

CMakeBuildConfiguration::CMakeBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("CMake"));

    buildDirectoryAspect()->setValueAcceptor(
        [](const QString &oldDir, const QString &newDir) -> std::optional<QString> {
            if (oldDir.isEmpty())
                return newDir;

            const FilePath oldDirCMakeCache = FilePath::fromUserInput(oldDir).pathAppended(
                Constants::CMAKE_CACHE_TXT);
            const FilePath newDirCMakeCache = FilePath::fromUserInput(newDir).pathAppended(
                Constants::CMAKE_CACHE_TXT);

            if (oldDirCMakeCache.exists() && !newDirCMakeCache.exists()) {
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

    // Will not be displayed, only persisted
    sourceDirectory.setSettingsKey("CMake.Source.Directory");

    buildTypeAspect.setSettingsKey(CMAKE_BUILD_TYPE);
    buildTypeAspect.setLabelText(Tr::tr("Build type:"));
    buildTypeAspect.setDisplayStyle(StringAspect::LineEditDisplay);
    buildTypeAspect.setDefaultValue("Unknown");

    additionalCMakeOptions.setSettingsKey("CMake.Additional.Options");
    additionalCMakeOptions.setLabelText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
    additionalCMakeOptions.setDisplayStyle(StringAspect::LineEditDisplay);

    macroExpander()->registerVariable(
        DEVELOPMENT_TEAM_FLAG, Tr::tr("The CMake flag for the development team"), [this] {
            const CMakeConfig flags = signingFlags();
            if (flags.contains(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM))
                return flags.value(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM).toArgument();
            return QString();
        });
    macroExpander()->registerVariable(
        PROVISIONING_PROFILE_FLAG, Tr::tr("The CMake flag for the provisioning profile"), [this] {
            const CMakeConfig flags = signingFlags();
            if (flags.contains(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER))
                return flags.value(CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER).toArgument();
            return QString();
        });

    macroExpander()->registerVariable(
        CMAKE_OSX_ARCHITECTURES_FLAG, Tr::tr("The CMake flag for the architecture on macOS"), [] {
            // TODO deprecated since Qt Creator 15, remove later
            return QString();
        });
    macroExpander()->registerVariable(
        QT_QML_DEBUG_FLAG, Tr::tr("The C++ compiler QML debugging define, if enabled"), [this] {
            if (aspect<QtSupport::QmlDebuggingAspect>()->value() == TriState::Enabled) {
                return QLatin1String(QT_QML_DEBUG_PARAM);
            }
            return QLatin1String();
        });

    macroExpander()->registerVariable(
        QT_ENABLE_QML_DEBUG_FLAG, Tr::tr("The CMake boolean value for QML debugging: ON / OFF."), [this] {
            if (aspect<QtSupport::QmlDebuggingAspect>()->value() == TriState::Enabled) {
                return QLatin1String("ON");
            }
            return QLatin1String("OFF");
        });

    setInitialBuildAndCleanSteps();

    setInitializer([this](const BuildInfo &info) {
        const Kit *k = kit();
        const QtSupport::QtVersion *qt = QtSupport::QtKitAspect::qtVersion(k);
        const Store extraInfoMap = storeFromVariant(info.extraInfo);
        const QString buildType = extraInfoMap.contains(CMAKE_BUILD_TYPE)
                                      ? extraInfoMap.value(CMAKE_BUILD_TYPE).toString()
                                      : info.typeName;

        CommandLine cmd = defaultInitialCMakeCommand(k, project(), buildType);
        cmakeBuildSystem()->setIsMultiConfig(CMakeGeneratorKitAspect::isMultiConfigGenerator(k));

        // Android magic:
        if (RunDeviceTypeKitAspect::deviceTypeId(k) == Android::Constants::ANDROID_DEVICE_TYPE) {
            auto addUniqueKeyToCmd = [&cmd] (const QString &prefix, const QString &value) -> bool {
                const bool isUnique =
                    !Utils::contains(cmd.splitArguments(), [&prefix] (const QString &arg) {
                                                               return arg.startsWith(prefix); });
                if (isUnique)
                    cmd.addArg(prefix + value);
                return isUnique;
            };
            buildSteps()->appendStep(Android::Constants::ANDROID_BUILD_APK_ID);
            const auto bs = buildSteps()->steps().constLast();
            addUniqueKeyToCmd("-DANDROID_PLATFORM:STRING=",
                              bs->data(Android::Constants::AndroidNdkPlatform).toString());
            auto ndkLocation = bs->data(Android::Constants::NdkLocation).value<FilePath>();
            cmd.addArg("-DANDROID_NDK:PATH=" + ndkLocation.path());

            cmd.addArg("-DCMAKE_TOOLCHAIN_FILE:FILEPATH="
                   + ndkLocation.pathAppended("build/cmake/android.toolchain.cmake").path());
            cmd.addArg("-DANDROID_USE_LEGACY_TOOLCHAIN_FILE:BOOL=OFF");

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
                if (qt->qtVersion() >= QVersionNumber(6, 1, 0)) {
                    cmd.addArg("-DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL:BOOL=ON");
                    if (qt->qtVersion() >= QVersionNumber(6, 8, 0))
                        cmd.addArg("-DQT_USE_TARGET_ANDROID_BUILD_DIR:BOOL=ON");
                }

                cmd.addArg("-DQT_HOST_PATH:PATH=%{Qt:QT_HOST_PREFIX}");
                cmd.addArg("-DANDROID_SDK_ROOT:PATH=" + sdkLocation.path());
            } else {
                cmd.addArg("-DANDROID_SDK:PATH=" + sdkLocation.path());
            }
        }

        const IDevice::ConstPtr device = RunDeviceKitAspect::device(k);
        if (CMakeBuildConfiguration::isIos(k)) {
            if (qt && qt->qtVersion().majorVersion() >= 6) {
                // TODO it would be better if we could set
                // CMAKE_SYSTEM_NAME=iOS and CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=YES
                // and build with "cmake --build . -- -arch <arch>" instead of setting the architecture
                // and sysroot in the CMake configuration, but that currently doesn't work with Qt/CMake
                // https://gitlab.kitware.com/cmake/cmake/-/issues/21276
                const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(k);
                const QString sysroot = deviceType == Ios::Constants::IOS_DEVICE_TYPE
                                            ? QLatin1String("iphoneos")
                                            : QLatin1String("iphonesimulator");
                cmd.addArg(CMAKE_QT6_TOOLCHAIN_FILE_ARG);
                cmd.addArg("-DCMAKE_OSX_SYSROOT:STRING=" + sysroot);
                cmd.addArg("%{" + QLatin1String(DEVELOPMENT_TEAM_FLAG) + "}");
                cmd.addArg("%{" + QLatin1String(PROVISIONING_PROFILE_FLAG) + "}");
            }
        }

        if (isWebAssembly(k) || isQnx(k) || isWindowsARM64(k) || isVxWorks(k)) {
            if (qt && qt->qtVersion().majorVersion() >= 6)
                cmd.addArg(CMAKE_QT6_TOOLCHAIN_FILE_ARG);
        }

        if (extraInfoMap.contains(Constants::CMAKE_HOME_DIR))
            sourceDirectory.setValue(FilePath::fromVariant(extraInfoMap.value(Constants::CMAKE_HOME_DIR)));

        if (extraInfoMap.contains(Constants::CMAKE_IMPORTED_BUILD))
            setExtraData(Constants::CMAKE_IMPORTED_BUILD, true);

        qmlDebugging.setValue(extraInfoMap.contains(Constants::QML_DEBUG_SETTING)
                                  ? TriState::fromVariant(extraInfoMap.value(Constants::QML_DEBUG_SETTING))
                                  : TriState::Default);

        if (qt && qt->isQmlDebuggingSupported())
            cmd.addArg(QLatin1String("-DQT_ENABLE_QML_DEBUG:BOOL=%{") + QT_ENABLE_QML_DEBUG_FLAG + "}");

        // QT_QML_GENERATE_QMLLS_INI, if enabled via the settings checkbox:
        if (isGenerateQmllsSettingsEnabled()) {
            cmd.addArg("-DQT_QML_GENERATE_QMLLS_INI:BOOL=ON");
        }

        CMakeProject *cmakeProject = static_cast<CMakeProject *>(project());
        configureEnv.setUserEnvironmentChanges(
            getEnvironmentItemsFromCMakeConfigurePreset(cmakeProject, k));

        QStringList initialCMakeArguments = cmd.splitArguments();
        addCMakeConfigurePresetToInitialArguments(initialCMakeArguments,
                                                  cmakeProject,
                                                  k,
                                                  configureEnvironment(),
                                                  buildDirectory());
        setInitialCMakeArguments(initialCMakeArguments);
        setCMakeBuildType(buildType);

        setBuildPresetToBuildSteps();
    });
}

CMakeBuildConfiguration::~CMakeBuildConfiguration() = default;

FilePath CMakeBuildConfiguration::shadowBuildDirectory(
    const FilePath &projectFilePath,
    const Kit *k,
    const QString &bcName,
    BuildConfiguration::BuildType buildType,
    bool expand)
{
    if (projectFilePath.isEmpty())
        return {};

    const QString projectName = CMakeProject::projectDisplayName(projectFilePath);
    FilePath buildPath = expand ? buildDirectoryFromTemplate(
                                      projectFilePath.absolutePath(),
                                      projectFilePath,
                                      projectName,
                                      k,
                                      bcName,
                                      buildType,
                                      "cmake")
                                : rawBuildDirectoryFromTemplate(k, projectFilePath);

    if (CMakeGeneratorKitAspect::isMultiConfigGenerator(k)) {
        // remove build config name, or the placeholder for it, including a potential leading dash
        // since for multiconfig generators the directory should best be the same for all configs
        QString path = buildPath.path();
        const QString toRemove = expand ? bcName : "%{BuildConfig:Name}";
        path.remove("-" + toRemove);
        path.remove(toRemove);
        buildPath = buildPath.withNewPath(path);
    }

    return buildPath;
}

bool CMakeBuildConfiguration::isIos(const Kit *k)
{
    const Id deviceType = RunDeviceTypeKitAspect::deviceTypeId(k);
    return deviceType == Ios::Constants::IOS_DEVICE_TYPE
           || deviceType == Ios::Constants::IOS_SIMULATOR_TYPE;
}

bool CMakeBuildConfiguration::hasQmlDebugging(const CMakeConfig &config)
{
    // Qt Creator 18 and lower method of setting Qml debug flag
    const QString cxxFlagsInit = config.stringValueOf(CMAKE_CXX_FLAGS_INIT);
    const QString cxxFlags = config.stringValueOf(CMAKE_CXX_FLAGS);
    const bool cxxFlagsQmlDebug = cxxFlagsInit.contains(QT_QML_DEBUG_PARAM)
                                  && cxxFlags.contains(QT_QML_DEBUG_PARAM);

    return config.contains(QT_ENABLE_QML_DEBUG)
               ? CMakeConfigItem::toBool(config.stringValueOf(QT_ENABLE_QML_DEBUG))
                     .value_or(cxxFlagsQmlDebug)
               : cxxFlagsQmlDebug;
}

void CMakeBuildConfiguration::buildTarget(const QString &buildTarget)
{
    auto cmBs = qobject_cast<CMakeBuildStep *>(findOrDefault(
                                                   buildSteps()->steps(),
                                                   [](const BuildStep *bs) {
        return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
    }));

    if (cmBs) {
        if (m_unrestrictedBuildTargets.isEmpty())
            m_unrestrictedBuildTargets = cmBs->buildTargets();
        cmBs->setBuildTargets({buildTarget});
    }

    BuildManager::buildList(buildSteps());

    if (cmBs) {
        cmBs->setBuildTargets(m_unrestrictedBuildTargets);
        m_unrestrictedBuildTargets.clear();
    }
}

void CMakeBuildConfiguration::reBuildTarget(const QString &cleanTarget, const QString &buildTarget)
{
    auto cmBs = qobject_cast<CMakeBuildStep *>(
        findOrDefault(buildSteps()->steps(), [](const BuildStep *bs) {
            return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
        }));
    auto cmCs = qobject_cast<CMakeBuildStep *>(
        findOrDefault(cleanSteps()->steps(), [](const BuildStep *bs) {
            return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
        }));

    if (cmBs) {
        if (m_unrestrictedBuildTargets.isEmpty())
            m_unrestrictedBuildTargets = cmBs->buildTargets();
        cmBs->setBuildTargets({buildTarget});
    }
    QString originalCleanTarget;
    if (cmCs) {
        originalCleanTarget = cmCs->cleanTarget();
        cmCs->setBuildTargets({cleanTarget});
    }

    BuildManager::buildLists({cleanSteps(), buildSteps()});

    if (cmBs) {
        cmBs->setBuildTargets(m_unrestrictedBuildTargets);
        m_unrestrictedBuildTargets.clear();
    }
    if (cmCs)
        cmCs->setBuildTargets({originalCleanTarget});
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
    const QList<CMakeConfigItem> filteredInitials = Utils::filtered(
        m_configurationChanges.toList(), [initialParameters](const CMakeConfigItem &ci) {
            return initialParameters ? ci.isInitial : !ci.isInitial;
        });
    return Utils::transform(filteredInitials, &CMakeConfigItem::toArgument);
}

CMakeConfig CMakeBuildSystem::initialCMakeConfiguration() const
{
    return cmakeBuildConfiguration()->initialCMakeArguments.cmakeConfiguration();
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

void CMakeBuildConfiguration::setInitialCMakeArguments(const QStringList &args)
{
    QStringList additionalArguments;
    initialCMakeArguments.setAllValues(args.join('\n'), additionalArguments);

    // Set the unknown additional arguments also for the "Current Configuration"
    setAdditionalCMakeArguments(additionalArguments);
}

QStringList CMakeBuildConfiguration::additionalCMakeArguments() const
{
    return ProcessArgs::splitArgs(additionalCMakeOptions(), HostOsInfo::hostOs());
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
    additionalCMakeOptions.setValue(ProcessArgs::joinArgs(nonEmptyAdditionalArguments));
}

void CMakeBuildConfiguration::filterConfigArgumentsFromAdditionalCMakeArguments()
{
    // On iOS the %{Ios:DevelopmentTeam:Flag} evalues to something like
    // -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM:STRING=MAGICSTRING
    // which is already part of the CMake variables and should not be also
    // in the addtional CMake options
    const QStringList arguments = ProcessArgs::splitArgs(additionalCMakeOptions(),
                                                         HostOsInfo::hostOs());
    QStringList unknownOptions;
    const CMakeConfig config = CMakeConfig::fromArguments(arguments, unknownOptions);

    additionalCMakeOptions.setValue(ProcessArgs::joinArgs(unknownOptions));
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
    TaskHub::addTask<BuildSystemTask>(Task::TaskType::Error, message);
    emit errorOccurred(m_error);
}

void CMakeBuildSystem::setWarning(const QString &message)
{
    if (m_warning == message)
        return;
    m_warning = message;
    TaskHub::addTask<BuildSystemTask>(Task::TaskType::Warning, message);
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

QWidget *CMakeBuildConfiguration::createConfigWidget()
{
    m_configWidget = new CMakeBuildSettingsWidget(this);
    return m_configWidget;
}

QStringList CMakeBuildConfiguration::initialCMakeOptions() const
{
    return initialCMakeArguments.allValues();
}

CMakeConfig CMakeBuildConfiguration::updateCMakeHelperConfig(const CMakeConfig &config)
{
    CMakeConfig updatedConfig = config;

    // Migrate from "package-manager/auto-setup.cmake" to "cmake-helper/qtcreator-project.cmake"
    const auto cmakeHelperParameter = getCMakeHelperParameter();
    if (updatedConfig.contains(cmakeHelperParameter.key)) {
        CMakeConfigItem &item = updatedConfig[cmakeHelperParameter.key];
        if (item.value.endsWith("auto-setup.cmake"))
            item.value = cmakeHelperParameter.value;
    }

    return updatedConfig;
}

void CMakeBuildConfiguration::setInitialArgs(const QStringList &args)
{
    setInitialCMakeArguments(args);
}

QStringList CMakeBuildConfiguration::initialArgs() const
{
    return initialCMakeOptions();
}

QStringList CMakeBuildConfiguration::additionalArgs() const
{
    return additionalCMakeArguments();
}

void CMakeBuildConfiguration::reconfigure()
{
    cmakeBuildSystem()->clearCMakeCache();
    if (QTC_GUARD(m_configWidget))
        m_configWidget->updateInitialCMakeArguments();
    cmakeBuildSystem()->runCMake();
}

void CMakeBuildConfiguration::stopReconfigure()
{
     cmakeBuildSystem()->stopCMakeRun();
}

CMakeConfig CMakeBuildConfiguration::signingFlags() const
{
    return {};
}


void CMakeBuildConfiguration::setInitialBuildAndCleanSteps()
{
    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(kit());

    int buildSteps = 1;
    if (!presetItem.isNull()) {
        const QString presetName = presetItem.expandedValue(kit());
        const CMakeProject *project = static_cast<const CMakeProject *>(this->project());

        const auto buildPresets = project->presetsData().buildPresets;
        const int count
            = std::count_if(buildPresets.begin(),
                            buildPresets.end(),
                            [presetName, project](const PresetsDetails::BuildPreset &preset) {
                                bool enabled = true;
                                if (preset.condition)
                                    enabled = CMakePresets::Macros::evaluatePresetCondition(
                                        preset, project->projectDirectory());

                                return preset.configurePreset == presetName && !preset.hidden
                                       && enabled;
                            });
        if (count != 0)
            buildSteps = count;
    }

    for (int i = 0; i < buildSteps; ++i)
        appendInitialBuildStep(Constants::CMAKE_BUILD_STEP_ID);

    appendInitialCleanStep(Constants::CMAKE_BUILD_STEP_ID);
}

void CMakeBuildConfiguration::setBuildPresetToBuildSteps()
{
    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(kit());

    if (presetItem.isNull())
        return;

    const QString presetName = presetItem.expandedValue(kit());
    const CMakeProject *project = static_cast<const CMakeProject *>(this->project());

    const auto allBuildPresets = project->presetsData().buildPresets;
    const auto buildPresets = Utils::filtered(
        allBuildPresets, [presetName, project](const PresetsDetails::BuildPreset &preset) {
            bool enabled = true;
            if (preset.condition)
                enabled = CMakePresets::Macros::evaluatePresetCondition(preset,
                                                                        project->projectDirectory());

            return preset.configurePreset == presetName && !preset.hidden && enabled;
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
            getEnvironmentItemsFromCMakeBuildPreset(project, kit(), buildPresets[i].name));

        if (buildPresets[i].targets) {
            QString targets = buildPresets[i].targets->join(" ");

            CMakePresets::Macros::expand(buildPresets[i],
                                         cbs->environment(),
                                         project->projectDirectory(),
                                         targets);

            cbs->setBuildTargets(targets.split(" "));
        }

        QStringList cmakeArguments;
        if (buildPresets[i].jobs)
            cmakeArguments.append(QString("-j %1").arg(*buildPresets[i].jobs));
        if (buildPresets[i].verbose && *buildPresets[i].verbose)
            cmakeArguments.append("--verbose");
        if (buildPresets[i].cleanFirst && *buildPresets[i].cleanFirst)
            cmakeArguments.append("--clean-first");
        if (!cmakeArguments.isEmpty())
            cbs->setCMakeArguments(cmakeArguments);

        if (buildPresets[i].nativeToolOptions) {
            QString nativeToolOptions = buildPresets[i].nativeToolOptions->join(" ");

            CMakePresets::Macros::expand(buildPresets[i],
                                         cbs->environment(),
                                         project->projectDirectory(),
                                         nativeToolOptions);

            cbs->setToolArguments(nativeToolOptions.split(" "));
        }

        if (buildPresets[i].configuration) {
            cbs->setConfiguration(*buildPresets[i].configuration);
            cbs->setStepEnabled(buildTypeAspect() == buildPresets[i].configuration);
        } else {
            // Leave only the first build step enabled
            if (i > 0)
                cbs->setStepEnabled(false);
        }
    }
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<CMakeBuildConfiguration>(Constants::CMAKE_BUILDCONFIGURATION_ID);

    setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setSupportedProjectMimeTypeName(Utils::Constants::CMAKE_PROJECT_MIMETYPE);

    setBuildGenerator([](const Kit *k, const FilePath &projectPath, bool forSetup) {
        QList<BuildInfo> result;

        // Skip the default shadow build directories for build types if we have presets
        const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(k);
        if (!presetItem.isNull())
            return result;

        for (int type = BuildTypeDebug; type != BuildTypeLast; ++type) {
            BuildInfo info = createBuildInfo(BuildType(type));
            info.projectName = CMakeProject::projectDisplayName(projectPath);
            if (forSetup) {
                info.buildDirectory = CMakeBuildConfiguration::shadowBuildDirectory(projectPath,
                                k,
                                info.typeName,
                                info.buildType,
                                false);
            } else {
                info.displayName.clear(); // ask for a name
                info.buildDirectory.clear(); // This depends on the displayName
            }
            info.enabledByDefault = type == BuildTypeDebug;
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
    info.buildSystemName = CMakeBuildSystem::name();

    switch (buildType) {
    case BuildTypeNone:
        info.typeName = "Build";
        info.displayName = msgBuildConfigurationBuild();
        info.buildType = BuildConfiguration::Unknown;
        break;
    case BuildTypeDebug: {
        info.typeName = "Debug";
        info.displayName = msgBuildConfigurationDebug();
        info.buildType = BuildConfiguration::Debug;
        Store extraInfo;
        // enable QML debugging by default
        extraInfo.insert(Constants::QML_DEBUG_SETTING, TriState::Enabled.toVariant());
        info.extraInfo = variantFromStore(extraInfo);
        break;
    }
    case BuildTypeRelease:
        info.typeName = "Release";
        info.displayName = msgBuildConfigurationRelease();
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
        info.displayName = msgBuildConfigurationProfile();
        info.buildType = BuildConfiguration::Profile;
        Store extraInfo;
        // override CMake build type, which defaults to info.typeName
        extraInfo.insert(CMAKE_BUILD_TYPE, "RelWithDebInfo");
        // enable QML debugging by default
        extraInfo.insert(Constants::QML_DEBUG_SETTING, TriState::Enabled.toVariant());
        info.extraInfo = variantFromStore(extraInfo);
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
    return cmakeBuildSystem()->buildType();
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

CMakeBuildSystem *CMakeBuildConfiguration::cmakeBuildSystem() const
{
    return qobject_cast<CMakeBuildSystem *>(buildSystem());
}

void CMakeBuildConfiguration::addToEnvironment(Environment &env) const
{
    // Use the user provided VCPKG_ROOT if existing
    // Visual C++ 2022 (and newer) come with their own VCPKG_ROOT
    // that is incompatible with Qt Creator
    const QString vcpkgRoot = qtcEnvironmentVariable(Constants::VCPKG_ROOT);
    if (!vcpkgRoot.isEmpty())
        env.set(Constants::VCPKG_ROOT, vcpkgRoot);

    const FilePath cmakeExecutable = CMakeKitAspect::cmakeExecutable(kit());
    // The hack further down is only relevant for desktop
    if (!cmakeExecutable.isEmpty() && !cmakeExecutable.isLocal())
        return;

    const FilePath ninja = settings(nullptr).ninjaPath();
    if (!ninja.isEmpty())
        env.appendOrSetPath(ninja.isFile() ? ninja.parentDir() : ninja);
}

void CMakeBuildConfiguration::restrictNextBuild(const ProjectExplorer::RunConfiguration *rc)
{
    setRestrictedBuildTarget(rc ? rc->buildKey() : QString());
}

void CMakeBuildConfiguration::setRestrictedBuildTarget(const QString &buildTarget)
{
    auto buildStep = qobject_cast<CMakeBuildStep *>(
        findOrDefault(buildSteps()->steps(), [](const BuildStep *bs) {
            return bs->id() == Constants::CMAKE_BUILD_STEP_ID;
        }));
    if (!buildStep)
        return;

    if (!buildTarget.isEmpty()) {
        if (m_unrestrictedBuildTargets.isEmpty())
            m_unrestrictedBuildTargets = buildStep->buildTargets();
        buildStep->setBuildTargets({buildTarget});
        return;
    }

    if (!m_unrestrictedBuildTargets.isEmpty()) {
        buildStep->setBuildTargets(m_unrestrictedBuildTargets);
        m_unrestrictedBuildTargets.clear();
    }
}

Environment CMakeBuildConfiguration::configureEnvironment() const
{
    Environment env = configureEnv.environment();
    addToEnvironment(env);

    return env;
}

QString CMakeBuildSystem::cmakeBuildType() const
{
    auto setBuildTypeFromConfig = [this](const CMakeConfig &config) {
        if (config.contains("CMAKE_BUILD_TYPE")) {
            const CMakeConfigItem item = config.value("CMAKE_BUILD_TYPE");
            if (!item.isInitial)
                cmakeBuildConfiguration()->setCMakeBuildType(QString::fromUtf8(item.value));
        }
    };

    if (!isMultiConfig())
        setBuildTypeFromConfig(configurationChanges());

    QString cmakeBuildType = cmakeBuildConfiguration()->buildTypeAspect();

    const Utils::FilePath cmakeCacheTxt = buildConfiguration()->buildDirectory().pathAppended(
        Constants::CMAKE_CACHE_TXT);
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

void CMakeBuildConfiguration::setCMakeBuildType(const QString &cmakeBuildType, bool quiet)
{
    buildTypeAspect.setValue(cmakeBuildType, quiet ? BaseAspect::BeQuiet : BaseAspect::DoEmit);
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

    initialCMakeArguments.append(ProcessArgs::splitArgs(value(), HostOsInfo::hostOs()));

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

    CMakeConfig config = CMakeConfig::fromArguments(arguments, additionalOptions);
    // Join CMAKE_CXX_FLAGS_INIT values if more entries are present, or skip the same
    // values like CMAKE_EXE_LINKER_FLAGS_INIT coming from both C and CXX compilers
    QHash<QByteArray, CMakeConfigItem> uniqueConfig;
    for (CMakeConfigItem &ci : config) {
        ci.isInitial = true;
        if (uniqueConfig.contains(ci.key)) {
            if (uniqueConfig[ci.key].value != ci.value)
                uniqueConfig[ci.key].value = uniqueConfig[ci.key].value + " " + ci.value;
        } else {
            uniqueConfig.insert(ci.key, ci);
        }
    }
    m_cmakeConfiguration = uniqueConfig.values();

    // Display the unknown arguments in "Additional CMake Options"
    const QString additionalOptionsValue = ProcessArgs::joinArgs(additionalOptions);
    setValue(additionalOptionsValue, BeQuiet);
}

void InitialCMakeArgumentsAspect::setCMakeConfiguration(const CMakeConfig &config)
{
    m_cmakeConfiguration = config;
    for (CMakeConfigItem &ci : m_cmakeConfiguration)
        ci.isInitial = true;
}

void InitialCMakeArgumentsAspect::fromMap(const Store &map)
{
    const QString value = map.value(settingsKey(), defaultValue()).toString();
    QStringList additionalArguments;
    setAllValues(value, additionalArguments);
}

void InitialCMakeArgumentsAspect::toMap(Store &map) const
{
    saveToMap(map, allValues().join('\n'), defaultValue(), settingsKey());
}

InitialCMakeArgumentsAspect::InitialCMakeArgumentsAspect(AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("CMake.Initial.Parameters");
    setLabelText(Tr::tr("Additional CMake <a href=\"options\">options</a>:"));
    setDisplayStyle(LineEditDisplay);
}

// -----------------------------------------------------------------------------
// ConfigureEnvironmentAspect:
// -----------------------------------------------------------------------------
class ConfigureEnvironmentAspectWidget final : public EnvironmentAspectWidget
{
public:
    ConfigureEnvironmentAspectWidget(ConfigureEnvironmentAspect *aspect, BuildConfiguration *bc)
        : EnvironmentAspectWidget(aspect)
    {
        envWidget()->setOpenTerminalFunc([bc](const Environment &env) {
            Core::FileUtils::openTerminal(bc->buildDirectory(), env);
        });
    }
};

ConfigureEnvironmentAspect::ConfigureEnvironmentAspect(BuildConfiguration *bc)
    : EnvironmentAspect(bc)
{
    setIsLocal(true);
    setAllowPrintOnRun(false);
    setConfigWidgetCreator([this, bc] { return new ConfigureEnvironmentAspectWidget(this, bc); });
    addSupportedBaseEnvironment(Tr::tr("Clean Environment"), {});
    setLabelText(Tr::tr("Base environment for the CMake configure step:"));

    const int systemEnvIndex = addSupportedBaseEnvironment(Tr::tr("System Environment"), [bc] {
        IDevice::ConstPtr device = BuildDeviceKitAspect::device(bc->kit());
        return device ? device->systemEnvironment() : Environment::systemEnvironment();
    });

    const int buildEnvIndex = addSupportedBaseEnvironment(Tr::tr("Build Environment"), [bc] {
        return bc->environment();
    });

    connect(bc, &BuildConfiguration::environmentChanged,
            this, &EnvironmentAspect::environmentChanged);

    const CMakeConfigItem presetItem = CMakeConfigurationKitAspect::cmakePresetConfigItem(
        bc->kit());

    setBaseEnvironmentBase(presetItem.isNull() ? buildEnvIndex : systemEnvIndex);

    connect(bc->project(),
            &Project::environmentChanged,
            this,
            &EnvironmentAspect::environmentChanged);

    connect(KitManager::instance(), &KitManager::kitUpdated, this, [this, bc](const Kit *k) {
        if (bc->kit() == k)
            emit EnvironmentAspect::environmentChanged();
    });

    addModifier([bc](Utils::Environment &env) {
        // This will add ninja to path
        bc->addToEnvironment(env);
        bc->kit()->addToBuildEnvironment(env);
        env.modify(bc->project()->additionalEnvironment());
    });
}

void ConfigureEnvironmentAspect::fromMap(const Store &map)
{
    // Match the key values from Qt Creator 9.0.0/1 to the ones from EnvironmentAspect
    const bool cleanSystemEnvironment = map.value(CLEAR_SYSTEM_ENVIRONMENT_KEY).toBool();
    const QStringList userEnvironmentChanges
        = map.value(USER_ENVIRONMENT_CHANGES_KEY).toStringList();

    const int baseEnvironmentIndex = map.value(BASE_ENVIRONMENT_KEY, baseEnvironmentBase()).toInt();

    Store tmpMap;
    tmpMap.insert(BASE_KEY, cleanSystemEnvironment ? 0 : baseEnvironmentIndex);
    tmpMap.insert(CHANGES_KEY, userEnvironmentChanges);

    ProjectExplorer::EnvironmentAspect::fromMap(tmpMap);
}

void ConfigureEnvironmentAspect::toMap(Store &map) const
{
    Store tmpMap;
    ProjectExplorer::EnvironmentAspect::toMap(tmpMap);

    const int baseKey = tmpMap.value(BASE_KEY).toInt();

    map.insert(CLEAR_SYSTEM_ENVIRONMENT_KEY, baseKey == 0);
    map.insert(BASE_ENVIRONMENT_KEY, baseKey);
    map.insert(USER_ENVIRONMENT_CHANGES_KEY, tmpMap.value(CHANGES_KEY).toStringList());
}


void setupCMakeBuildConfiguration()
{
    static CMakeBuildConfigurationFactory theCMakeBuildConfigurationFactory;
}

} // namespace Internal
} // namespace CMakeProjectManager
