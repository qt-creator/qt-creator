// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runconfigurationaspects.h"

#include "buildmanager.h"
#include "buildpropertiessettings.h"
#include "devicesupport/devicekitaspects.h"
#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "environmentaspect.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QPushButton>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Utils;
using namespace Layouting;

namespace ProjectExplorer {

/*!
    \class ProjectExplorer::TerminalAspect
    \inmodule QtCreator

    \brief The TerminalAspect class lets a user specify that an executable
    should be run in a separate terminal.

    The initial value is provided as a hint from the build systems.
*/

TerminalAspect::TerminalAspect(AspectContainer *container)
    : BaseAspect(container)
{
    setDisplayName(Tr::tr("Terminal"));
    setId("TerminalAspect");
    setSettingsKey("RunConfiguration.UseTerminal");
    addDataExtractor(this, &TerminalAspect::useTerminal, &Data::useTerminal);
    addDataExtractor(this, &TerminalAspect::isUserSet, &Data::isUserSet);

    calculateUseTerminal();
    ProjectExplorerSettings::registerCallback(this, &ProjectExplorerSettings::terminalMode, [this] {
        calculateUseTerminal();
    });
}

/*!
    \reimp
*/
void TerminalAspect::addToLayoutImpl(Layout &parent)
{
    auto checkBox = createSubWidget<QCheckBox>(Tr::tr("Run in terminal"));
    checkBox->setChecked(m_useTerminal);
    checkBox->setEnabled(isEnabled());
    parent.addItems({empty, checkBox});
    connect(checkBox, &QAbstractButton::clicked, this, [this, checkBox] {
        m_userSet = true;
        m_useTerminal = checkBox->isChecked();
        emit useTerminalChanged(QPrivateSignal());
        emit changed();
    });
    connect(this, &TerminalAspect::useTerminalChanged, checkBox, [this, checkBox] {
        checkBox->setChecked(m_useTerminal);
    });
}

/*!
    \reimp
*/
void TerminalAspect::fromMap(const Store &map)
{
    if (map.contains(settingsKey())) {
        m_useTerminal = map.value(settingsKey()).toBool();
        m_userSet = true;
    } else {
        m_userSet = false;
    }

    emit useTerminalChanged(QPrivateSignal());
}

/*!
    \reimp
*/
void TerminalAspect::toMap(Store &data) const
{
    if (m_userSet)
        data.insert(settingsKey(), m_useTerminal);
}

void TerminalAspect::calculateUseTerminal()
{
    if (m_userSet)
        return;
    bool useTerminal;
    switch (ProjectExplorerSettings::get(container()).terminalMode()) {
    case TerminalMode::On: useTerminal = true; break;
    case TerminalMode::Off: useTerminal = false; break;
    default: useTerminal = m_useTerminalHint;
    }
    if (m_useTerminal != useTerminal) {
        m_useTerminal = useTerminal;
        emit changed();
    }
    emit useTerminalChanged(QPrivateSignal());
}

/*!
    Returns whether a separate terminal should be used.
*/
bool TerminalAspect::useTerminal() const
{
    return m_useTerminal;
}

/*!
    Sets the initial value to \a hint.
*/
void TerminalAspect::setUseTerminalHint(bool hint)
{
    m_useTerminalHint = hint;
    calculateUseTerminal();
}

QVariant TerminalAspect::variantValue() const
{
    return m_useTerminal;
}

void TerminalAspect::setVariantValue(const QVariant &value, Announcement howToAnnounce)
{
    const bool useTerminal = value.toBool();
    if (m_userSet && m_useTerminal == useTerminal)
        return;
    m_userSet = true;
    m_useTerminal = useTerminal;
    emit useTerminalChanged(QPrivateSignal());
    if (howToAnnounce == DoEmit)
        emit changed();
}

/*!
    Returns whether the user set the value.
*/
bool TerminalAspect::isUserSet() const
{
    return m_userSet;
}

/*!
    \class ProjectExplorer::WorkingDirectoryAspect
    \inmodule QtCreator

    \brief The WorkingDirectoryAspect class lets the user specify a
    working directory for running the executable.
*/

WorkingDirectoryAspect::WorkingDirectoryAspect(AspectContainer *container)
    : BaseAspect(container)
{
    setDisplayName(Tr::tr("Working Directory"));
    setLabelText(Tr::tr("Working directory:"));
    setId("WorkingDirectoryAspect");
    setSettingsKey("RunConfiguration.WorkingDirectory");
    setDefaultWorkingDirectory(
        FilePath::fromUserInput(buildPropertiesSettings().workingDirectoryTemplate.value()));
}

void WorkingDirectoryAspect::setEnvironment(EnvironmentAspect *envAspect)
{
    m_envAspect = envAspect;
}

/*!
    \reimp
*/
void WorkingDirectoryAspect::addToLayoutImpl(Layout &builder)
{
    auto chooser = new PathChooser;
    if (QTC_GUARD(macroExpander()))
        chooser->setMacroExpander(macroExpander());
    chooser->setHistoryCompleter(settingsKey());
    chooser->setExpectedKind(Utils::PathChooserKind::ExistingDirectory);
    chooser->setPromptDialogTitle(Tr::tr("Select Working Directory"));
    chooser->setBaseDirectory(m_defaultWorkingDirectory);
    chooser->setFilePath(m_workingDirectory.isEmpty() ? m_defaultWorkingDirectory : m_workingDirectory);

    auto resetButton = new QToolButton;
    resetButton->setToolTip(Tr::tr("Reset to Default"));
    resetButton->setIcon(Utils::Icons::RESET.icon());
    connect(resetButton, &QAbstractButton::clicked, this, &WorkingDirectoryAspect::resetPath);

    connect(chooser, &PathChooser::textChanged, this, [this, chooser, resetButton] {
        m_workingDirectory = chooser->unexpandedFilePath();
        resetButton->setEnabled(m_workingDirectory != m_defaultWorkingDirectory);
        emit workingDirectoryChanged(QPrivateSignal());
    });
    connect(this, &WorkingDirectoryAspect::workingDirectoryChanged, chooser, [this, chooser] {
        if (chooser->unexpandedFilePath() != m_workingDirectory)
            chooser->setFilePath(m_workingDirectory);
    });
    connect(this, &WorkingDirectoryAspect::defaultWorkingDirectoryChanged, chooser, [this, chooser] {
        chooser->setBaseDirectory(m_defaultWorkingDirectory);
    });

    if (m_envAspect) {
        // The chooser is the context, so a torn-down panel stops asking instead
        // of outliving its widgets. Evaluating the environment still reaches into
        // device and kit machinery that can tear the tree down while it runs,
        // hence the check after it and not before.
        connect(m_envAspect, &EnvironmentAspect::environmentChanged, chooser,
                [envAspect = m_envAspect, chooser = QPointer(chooser)] {
            const Environment env = envAspect->environment();
            if (chooser)
                chooser->setEnvironment(env);
        });
        chooser->setEnvironment(m_envAspect->environment());
    }

    chooser->setReadOnly(isReadOnly());
    resetButton->setEnabled(!isReadOnly());
    registerSubWidget(chooser);
    registerSubWidget(resetButton);
    addLabeledItems(builder, {chooser, resetButton});
}

void WorkingDirectoryAspect::resetPath()
{
    m_workingDirectory = m_defaultWorkingDirectory;
    emit workingDirectoryChanged(QPrivateSignal());
}

/*!
    \reimp
*/
void WorkingDirectoryAspect::fromMap(const Store &map)
{
    m_workingDirectory = FilePath::fromString(map.value(settingsKey()).toString());
    m_defaultWorkingDirectory = FilePath::fromString(map.value(settingsKey() + ".default").toString());

    if (m_workingDirectory.isEmpty())
        m_workingDirectory = m_defaultWorkingDirectory;

    emit defaultWorkingDirectoryChanged(QPrivateSignal());
    emit workingDirectoryChanged(QPrivateSignal());
}

/*!
    \reimp
*/
void WorkingDirectoryAspect::toMap(Store &data) const
{
    const QString wd = m_workingDirectory == m_defaultWorkingDirectory
        ? QString() : m_workingDirectory.toUrlishString();
    saveToMap(data, wd, QString(), settingsKey());
    saveToMap(data, m_defaultWorkingDirectory.toUrlishString(), QString(), settingsKey() + ".default");
}

/*!
    Returns the selected directory.

    Macros in the value are expanded using \a expander.
*/
FilePath WorkingDirectoryAspect::workingDirectory() const
{
    const FilePath workingDir = macroExpander()->expand(m_workingDirectory);
    if (m_envAspect)
        return m_envAspect->environment().expandVariables(workingDir);
    return workingDir.deviceEnvironment().expandVariables(workingDir);
}

FilePath WorkingDirectoryAspect::defaultWorkingDirectory() const
{
    return m_defaultWorkingDirectory;
}

/*!
    Returns the selected directory.

    Macros in the value are not expanded.
*/
FilePath WorkingDirectoryAspect::unexpandedWorkingDirectory() const
{
    return m_workingDirectory;
}

/*!
    Sets the selected directory to \a workingDirectory.

    Macros in \a workingDirectory are kept unexpanded, as if it had been typed
    into the path chooser.
*/
void WorkingDirectoryAspect::setWorkingDirectory(const FilePath &workingDirectory)
{
    if (workingDirectory != m_workingDirectory) {
        m_workingDirectory = workingDirectory;
        emit changed();
    }
    emit workingDirectoryChanged(QPrivateSignal());
}

/*!
    Sets the default value to \a defaultWorkingDir.
*/
void WorkingDirectoryAspect::setDefaultWorkingDirectory(const FilePath &defaultWorkingDir)
{
    if (defaultWorkingDir == m_defaultWorkingDirectory)
        return;
    Utils::FilePath oldDefaultDir = m_defaultWorkingDirectory;
    m_defaultWorkingDirectory = defaultWorkingDir;
    emit defaultWorkingDirectoryChanged(QPrivateSignal());
    if (m_workingDirectory.isEmpty() || m_workingDirectory == oldDefaultDir) {
        m_workingDirectory = defaultWorkingDir;
        emit workingDirectoryChanged(QPrivateSignal());
    }
}

/*!
    \class ProjectExplorer::ArgumentsAspect
    \inmodule QtCreator

    \brief The ArgumentsAspect class lets a user specify command line
    arguments for an executable.
*/

ArgumentsAspect::ArgumentsAspect(AspectContainer *container)
    : BaseAspect(container)
{
    setDisplayName(Tr::tr("Arguments"));
    setLabelText(Tr::tr("Command line arguments:"));
    setId("ArgumentsAspect");
    setSettingsKey("RunConfiguration.Arguments");

    addDataExtractor(this, &ArgumentsAspect::arguments, &Data::arguments);
}

/*!
    Returns the main value of this aspect.

    Macros in the value are expanded using \a expander.
*/
QString ArgumentsAspect::arguments() const
{
    if (m_currentlyExpanding)
        return m_arguments;

    m_currentlyExpanding = true;
    const Result<QString> expanded = macroExpander()->expandProcessArgs(m_arguments);
    QTC_ASSERT_RESULT(expanded, return m_arguments);

    m_currentlyExpanding = false;
    return *expanded;
}

/*!
    Returns the main value of this aspect.

    Macros in the value are not expanded.
*/
QString ArgumentsAspect::unexpandedArguments() const
{
    return m_arguments;
}

/*!
    Sets the main value of this aspect to \a arguments.
*/
void ArgumentsAspect::setArguments(const QString &arguments)
{
    if (arguments != m_arguments) {
        m_arguments = arguments;
        emit changed();
    }
    emit argumentsChanged(QPrivateSignal());
}

/*!
    Adds a button to reset the main value of this aspect to the value
    computed by \a resetter.
*/
void ArgumentsAspect::setResetter(const std::function<QString()> &resetter)
{
    m_resetter = resetter;
}

/*!
    Resets the main value of this aspect.
*/
void ArgumentsAspect::resetArguments()
{
    QString arguments;
    if (m_resetter)
        arguments = m_resetter();
    setArguments(arguments);
}

/*!
    \reimp
*/
void ArgumentsAspect::fromMap(const Store &map)
{
    QVariant args = map.value(settingsKey());
    // Until 3.7 a QStringList was stored for Remote Linux
    if (args.typeId() == QMetaType::QStringList)
        m_arguments = ProcessArgs::joinArgs(args.toStringList(), OsTypeLinux);
    else
        m_arguments = args.toString();

    m_multiLine = map.value(settingsKey() + ".multi", false).toBool();
    emit multiLineChanged(QPrivateSignal());
    emit argumentsChanged(QPrivateSignal());
}

/*!
    \reimp
*/
void ArgumentsAspect::toMap(Store &map) const
{
    saveToMap(map, m_arguments, QString(), settingsKey());
    saveToMap(map, m_multiLine, false, settingsKey() + ".multi");
}

/*!
    \internal
*/
QWidget *ArgumentsAspect::createEditor()
{
    if (m_multiLine) {
        auto editor = new QPlainTextEdit;
        editor->setPlainText(m_arguments);
        editor->setReadOnly(isReadOnly());
        connect(editor, &QPlainTextEdit::textChanged, this, [this, editor] {
            setArguments(editor->toPlainText());
        });
        connect(this, &ArgumentsAspect::argumentsChanged, editor, [this, editor] {
            if (editor->toPlainText() != m_arguments)
                editor->setPlainText(m_arguments);
        });
        connect(this, &ArgumentsAspect::focusRequested, editor, [editor] { editor->setFocus(); });
        return editor;
    }
    auto editor = new FancyLineEdit;
    editor->setHistoryCompleter(settingsKey());
    editor->setText(m_arguments);
    editor->setReadOnly(isReadOnly());
    connect(editor, &QLineEdit::textChanged, this, &ArgumentsAspect::setArguments);
    connect(this, &ArgumentsAspect::argumentsChanged, editor, [this, editor] {
        if (editor->text() != m_arguments)
            editor->setText(m_arguments);
    });
    connect(this, &ArgumentsAspect::focusRequested, editor, [editor] { editor->setFocus(); });
    return editor;
}

/*!
    \reimp
*/
void ArgumentsAspect::addToLayoutImpl(Layout &builder)
{
    const auto container = new QWidget;
    const auto containerLayout = new QHBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(createEditor());

    // The editor is always the first item of the container.
    connect(this, &ArgumentsAspect::multiLineChanged, container, [this, containerLayout] {
        QWidget *oldEditor = containerLayout->itemAt(0)->widget();
        QTC_ASSERT(oldEditor, return);
        if (bool(qobject_cast<QPlainTextEdit *>(oldEditor)) == m_multiLine)
            return;
        delete containerLayout->replaceWidget(oldEditor, createEditor());
        delete oldEditor;
    });

    auto multiLineButton = new ExpandButton;
    multiLineButton->setToolTip(Tr::tr("Toggle multi-line mode."));
    multiLineButton->setChecked(m_multiLine);
    connect(multiLineButton, &QCheckBox::clicked, this, [this](bool checked) {
        if (m_multiLine == checked)
            return;
        m_multiLine = checked;
        emit multiLineChanged(QPrivateSignal());
    });
    connect(this, &ArgumentsAspect::multiLineChanged, multiLineButton, [this, multiLineButton] {
        multiLineButton->setChecked(m_multiLine);
    });
    containerLayout->addWidget(multiLineButton);
    containerLayout->setAlignment(multiLineButton, Qt::AlignTop);

    if (m_resetter) {
        auto resetButton = new QToolButton;
        resetButton->setToolTip(Tr::tr("Reset to Default"));
        resetButton->setIcon(Icons::RESET.icon());
        connect(resetButton, &QAbstractButton::clicked, this, &ArgumentsAspect::resetArguments);
        containerLayout->addWidget(resetButton);
        containerLayout->setAlignment(resetButton, Qt::AlignTop);
    }

    registerSubWidget(container);
    addLabeledItem(builder, container);
}

void ArgumentsAspect::setFocusToInputField()
{
    emit focusRequested(QPrivateSignal());
}

/*!
    \class ProjectExplorer::ExecutableAspect
    \inmodule QtCreator

    \brief The ExecutableAspect class provides a building block to provide an
    executable for a RunConfiguration.

    It combines a StringAspect that is typically updated automatically
    by the build system's parsing results with an optional manual override.
*/

ExecutableAspect::ExecutableAspect(AspectContainer *container)
    : BaseAspect(container)
{
    setDisplayName(Tr::tr("Executable"));
    setId("ExecutableAspect");
    setReadOnly(true);
    addDataExtractor(this, &ExecutableAspect::executable, &Data::executable);

    m_executable.setPlaceHolderText(Tr::tr("Enter the path to the executable"));
    m_executable.setLabelText(Tr::tr("Executable:"));

    connect(&m_executable, &StringAspect::changed, this, &ExecutableAspect::changed);
}

/*!
    \internal
*/

static IDevice::ConstPtr executionDevice(const Kit *k,
                                         ExecutableAspect::ExecutionDeviceSelector selector)
{
    if (k) {
        if (selector == ExecutableAspect::RunDevice)
            return RunDeviceKitAspect::device(k);
        if (selector == ExecutableAspect::BuildDevice)
            return BuildDeviceKitAspect::device(k);
    }
    return DeviceManager::defaultDesktopDevice();
}

ExecutableAspect::~ExecutableAspect()
{
    delete m_alternativeExecutable;
    m_alternativeExecutable = nullptr;
}

void ExecutableAspect::setDeviceSelector(Kit *kit, ExecutionDeviceSelector selector)
{
    m_kit = kit;
    m_selector = selector;

    const IDevice::ConstPtr dev = executionDevice(m_kit, m_selector);
    const OsType osType = dev ? dev->osType() : HostOsInfo::hostOs();

    m_executable.setDisplayFilter([osType](const QString &pathName) {
        return OsSpecificAspects::pathWithNativeSeparators(osType, pathName);
    });
}

/*!
   Sets the settings key for history completion to \a historyCompleterKey.

   \sa Utils::PathChooser::setHistoryCompleter()
*/
void ExecutableAspect::setHistoryCompleter(const Key &historyCompleterKey)
{
    m_executable.setHistoryCompleter(historyCompleterKey);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setHistoryCompleter(historyCompleterKey);
}

/*!
   Sets the acceptable kind of path values to \a expectedKind.

   \sa Utils::PathChooser::setExpectedKind()
*/
void ExecutableAspect::setExpectedKind(const PathChooserKind expectedKind)
{
    m_executable.setExpectedKind(expectedKind);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setExpectedKind(expectedKind);
}

/*!
   Sets the environment in which paths will be searched when the expected kind
   of paths is chosen as PathChooserKind::Command or PathChooserKind::ExistingCommand
   to \a env.
*/
void ExecutableAspect::setEnvironment(const Environment &env)
{
    m_executable.setEnvironment(env);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setEnvironment(env);
}

void ExecutableAspect::setReadOnly(bool readOnly)
{
    BaseAspect::setReadOnly(readOnly);
    m_executable.setReadOnly(readOnly);
}

void ExecutableAspect::setFocusToInputField()
{
    m_executable.setFocusToInputField();
}

/*!
   Makes an auto-detected executable overridable by the user.

   The \a overridingKey specifies the settings key for the user-provided executable,
   the \a useOverridableKey the settings key for the fact that it
   is actually overridden the user.

   \sa Utils::StringAspect::makeCheckable()
*/
void ExecutableAspect::makeOverridable(const Key &overridingKey, const Key &useOverridableKey)
{
    QTC_ASSERT(!m_alternativeExecutable, return);
    m_alternativeExecutable = new FilePathAspect;
    m_alternativeExecutable->setLabelText(Tr::tr("Alternate executable on device:"));
    m_alternativeExecutable->setSettingsKey(overridingKey);
    m_alternativeExecutable->makeCheckable(CheckBoxPlacement::Right,
                                           Tr::tr("Use this command instead"), useOverridableKey);
    connect(m_alternativeExecutable, &StringAspect::changed,
            this, &ExecutableAspect::changed);
}

/*!
    Returns the path of the executable specified by this aspect. In case
    the user selected a manual override this will be the value specified
    by the user.

    \sa makeOverridable()
 */
FilePath ExecutableAspect::executable() const
{
    FilePath exe = m_alternativeExecutable && m_alternativeExecutable->isChecked()
            ? (*m_alternativeExecutable)()
            : m_executable();

    if (const IDevice::ConstPtr dev = executionDevice(m_kit, m_selector))
        exe = dev->rootPath().withNewMappedPath(exe);

    return exe;
}

/*!
    \reimp
*/
void ExecutableAspect::addToLayoutImpl(Layout &builder)
{
    builder.addItem(m_executable);
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            &m_executable, &FilePathAspect::validateInput, Qt::UniqueConnection);
    if (m_alternativeExecutable) {
        builder.flush();
        builder.addItem(m_alternativeExecutable);
    }
}

/*!
    Sets the label text for the main chooser to
    \a labelText.

    \sa Utils::StringAspect::setLabelText()
*/
void ExecutableAspect::setLabelText(const QString &labelText)
{
    m_executable.setLabelText(labelText);
}

/*!
    Sets the place holder text for the main chooser to
    \a placeHolderText.

    \sa Utils::StringAspect::setPlaceHolderText()
*/
void ExecutableAspect::setPlaceHolderText(const QString &placeHolderText)
{
    m_executable.setPlaceHolderText(placeHolderText);
}

/*!
    Sets the value of the main chooser to \a executable.
*/
void ExecutableAspect::setExecutable(const FilePath &executable)
{
   m_executable.setValue(executable);
   m_executable.setShowToolTipOnLabel(true);
}

/*!
    Sets the settings key to \a key.
*/
void ExecutableAspect::setSettingsKey(const Key &key)
{
    BaseAspect::setSettingsKey(key);
    m_executable.setSettingsKey(key);
}

/*!
  \reimp
*/
void ExecutableAspect::fromMap(const Store &map)
{
    m_executable.fromMap(map);
    if (m_alternativeExecutable)
        m_alternativeExecutable->fromMap(map);
}

/*!
   \reimp
*/
void ExecutableAspect::toMap(Store &map) const
{
    m_executable.toMap(map);
    if (m_alternativeExecutable)
        m_alternativeExecutable->toMap(map);
}

/*!
    \class ProjectExplorer::UseLibraryPathsAspect
    \inmodule QtCreator

    \brief The UseLibraryPathsAspect class lets a user specify whether build
    library search paths should be added to the relevant environment
    variables.

    This modifies DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH on Mac, PATH
    on Windows and LD_LIBRARY_PATH everywhere else.
*/
UseLibraryPathsAspect::UseLibraryPathsAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setId("UseLibraryPath");
    setSettingsKey("RunConfiguration.UseLibrarySearchPath");
    setOsType(HostOsInfo::hostOs());
    setValue(ProjectExplorerSettings::get(container).addLibraryPathsToRunEnv());
}

void UseLibraryPathsAspect::setOsType(OsType osType)
{
    switch (osType) {
    case OsTypeMac:
        setLabel(Tr::tr("Add build library search path to DYLD_LIBRARY_PATH and "
                        "DYLD_FRAMEWORK_PATH"), LabelPlacement::AtCheckBox);
        break;
    case OsTypeWindows:
        setLabel(Tr::tr("Add build library search path to PATH"), LabelPlacement::AtCheckBox);
        break;
    default:
        setLabel(Tr::tr("Add build library search path to LD_LIBRARY_PATH"),
                 LabelPlacement::AtCheckBox);
        break;
    }
}

/*!
    \class ProjectExplorer::UseVncDisplayAspect
    \inmodule QtCreator

    \brief The UseVncDisplayAspect class lets a user run the application
    with the Qt VNC platform plugin.

    Sets the \c QT_QPA_PLATFORM environment variable to \c vnc.
    It is mostly useful when running Qt applications on remote devices as an
    alternative to X11 forwarding.
*/
UseVncDisplayAspect::UseVncDisplayAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setId("UseVncDisplay");
    setSettingsKey("RunConfiguration.UseVncDisplay");
    setLabel(Tr::tr("Use the Qt VNC platform for display"), LabelPlacement::AtCheckBox);
    setToolTip(
        Tr::tr(
            "Sets QT_QPA_PLATFORM=vnc which forwards the Qt application's UI to a built-in "
            "VNC server. Connect to the application UI with a VNC client. See the application "
            "output for details."));
}

/*!
    \class ProjectExplorer::UseDyldSuffixAspect
    \inmodule QtCreator

    \brief The UseDyldSuffixAspect class lets a user specify whether the
    DYLD_IMAGE_SUFFIX environment variable should be used on Mac.
*/

UseDyldSuffixAspect::UseDyldSuffixAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setId("UseDyldSuffix");
    setSettingsKey("RunConfiguration.UseDyldImageSuffix");
    setLabel(Tr::tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"),
             LabelPlacement::AtCheckBox);
    setVisible(HostOsInfo::isMacHost());
}

void UseDyldSuffixAspect::applyTo(EnvironmentAspect &environment)
{
    if (!HostOsInfo::isMacHost())
        return;

    connect(this, &UseDyldSuffixAspect::changed,
            &environment, &EnvironmentAspect::environmentChanged);
    environment.addModifier([this](Environment &env) {
        if (value())
            env.set("DYLD_IMAGE_SUFFIX", "_debug");
    });
}

/*!
    \class ProjectExplorer::RunAsAspect
    \inmodule QtCreator

    \brief The RunAsAspect class lets a user specify that the
    application should run under a specific account.
*/
RunAsAspect::RunAsAspect(Utils::AspectContainer *container) : AspectContainer(container)
{
    setId("RunAs");
    setDisplayName(Tr::tr("Run as User"));
    setLabelText(Tr::tr("Run as user:"));

    m_selection.setId("RunAsSelection");
    m_selection.setSettingsKey("RunConfiguration.RunAsRoot"); // Backward compat.
    m_selection.addOption(Tr::tr("Default"));
    m_selection.addOption(Tr::tr("root"));
    m_selection.addOption(Tr::tr("Other"));
    m_selection.setDefaultValue(0);
    m_user.setId("RunAsName");
    m_user.setSettingsKey("RunConfiguration.RunAsName");
    m_user.setDisplayStyle(StringAspect::LineEditDisplay);

    updateUserNameEnabled();
    connect(&m_selection, &SelectionAspect::changed, this, &RunAsAspect::updateUserNameEnabled);

    // Not technically correct, but sensible approximation.
    // Client code with more context can override.
    setVisible(HostOsInfo::isAnyUnixHost());
}

QString RunAsAspect::user() const
{
    switch (m_selection()) {
    case 0:
        break;
    case 1:
        return "root";
    case 2:
        return m_user();
    }
    return {};
}

void RunAsAspect::setUser(const QString &user)
{
    if (user.isEmpty()) {
        m_selection.setValue(0);
    } else if (user == "root") {
        m_selection.setValue(1);
    } else {
        m_selection.setValue(2);
        m_user.setValue(user);
    }
}

void RunAsAspect::fromMap(const Utils::Store &map)
{
    AspectContainer::fromMap(map);
    updateUserNameEnabled();
}

void RunAsAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    parent.addItems({createLabel(), m_selection, m_user, st});
}

void RunAsAspect::updateUserNameEnabled()
{
    m_user.setEnabled(m_selection.value() == 2);
}

/*!
    \class ProjectExplorer::EnableCategoriesFilterAspect
    \inmodule QtCreator

    \brief The EnableCategoriesFilterAspect class lets a user specify whether
    the application output should show the categories filtering widget.
*/

EnableCategoriesFilterAspect::EnableCategoriesFilterAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setId("EnableCategoriesFilter");
    setSettingsKey("RunConfiguration.EnableCategoriesFilter");
    setLabel(Tr::tr("Enable logging category filtering"), LabelPlacement::AtCheckBox);
    setToolTip(
        Tr::tr(
            "Enables filtering for logging categories (QLoggingCategory) in the Application "
            "Output. Requires Qt 6.11 or later."));
}

Interpreter::Interpreter()
    : id(QUuid::createUuid().toString())
{}

Interpreter::Interpreter(const QString &_id,
                         const QString &_name,
                         const FilePath &_command,
                         const DetectionSource &_detectionSource)
    : id(_id)
    , name(_name)
    , command(_command)
    , detectionSource(_detectionSource)
{}

void Interpreter::fromMap(const Utils::Store &store)
{
    id = store.value("Interpreter.id").toString();
    name = store.value("Interpreter.name").toString();
    command = FilePath::fromSettings(store.value("Interpreter.command"));
    detectionSource.fromMap(store);
}

void Interpreter::toMap(Utils::Store &store) const
{
    store.insert("Interpreter.id", id);
    store.insert("Interpreter.name", name);
    store.insert("Interpreter.command", command.toSettings());
    detectionSource.toMap(store);
}

static QString launcherType2UiString(const QString &type)
{
    if (type == "test")
        return Tr::tr("Test");
    else if (type == "emulator")
        return Tr::tr("Emulator");
    return QString();
}

Launcher::Launcher(const LauncherInfo &launcherInfo, const FilePath &sourceDirectory)
    : id(launcherInfo.type)
    , arguments(launcherInfo.arguments)
{
    if (launcherInfo.type != "unused") {
        command = launcherInfo.command;
        if (command.isRelativePath())
            command = sourceDirectory.resolvePath(command);
        displayName = QString("%1 (%2)").arg(launcherType2UiString(launcherInfo.type),
                                      CommandLine(command, arguments).displayName());
    }
}

Launcher::Launcher(const LauncherInfo &testLauncherInfo, const LauncherInfo &emulatorLauncherInfo, const Utils::FilePath &sourceDirectory)
    : id(testLauncherInfo.type + " + " + emulatorLauncherInfo.type)
    , command(testLauncherInfo.command)
    , arguments(testLauncherInfo.arguments)
{
    if (command.isRelativePath())
        command = sourceDirectory.resolvePath(command);
    FilePath command1 = emulatorLauncherInfo.command;
    if (command1.isRelativePath())
        command1 = sourceDirectory.resolvePath(command1);
    arguments.append(command1.toUrlishString());
    arguments.append(emulatorLauncherInfo.arguments);
    displayName = QString("%1 + %2 (%3)").arg(launcherType2UiString(testLauncherInfo.type),
                                       launcherType2UiString(emulatorLauncherInfo.type),
                                       CommandLine(command, arguments).displayName());
}

/*!
\class ProjectExplorer::LauncherAspect
\inmodule QtCreator

\brief With the LauncherAspect class, a user can specify a launcher program for
use with executable files for which a launcher program is optionally available.
*/

LauncherAspect::LauncherAspect(AspectContainer *container)
    : BaseAspect(container)
{
    addDataExtractor(this, &LauncherAspect::currentLauncher, &Data::launcher);
}

Launcher LauncherAspect::currentLauncher() const
{
    return Utils::findOrDefault(m_launchers, Utils::equal(&Launcher::id, m_currentId));
}

void LauncherAspect::updateLaunchers(const QList<Launcher> &launchers)
{
    if (m_launchers == launchers)
        return;
    m_launchers = launchers;
    ensureOfferedCurrentLauncher();
    emit launchersChanged(QPrivateSignal());
}

void LauncherAspect::setDefaultLauncher(const Launcher &launcher)
{
    if (m_defaultId == launcher.id)
        return;
    m_defaultId = launcher.id;
    if (m_currentId.isEmpty())
        setCurrentLauncher(launcher);
}

void LauncherAspect::setCurrentLauncher(const Launcher &launcher)
{
    if (!m_launchers.isEmpty() && !m_launchers.contains(launcher))
        return;
    setCurrentLauncherId(launcher.id);
}

void LauncherAspect::fromMap(const Store &map)
{
    setCurrentLauncherId(map.value(settingsKey(), m_defaultId).toString());
    ensureOfferedCurrentLauncher();
}

void LauncherAspect::toMap(Store &map) const
{
    if (m_currentId != m_defaultId)
        saveToMap(map, m_currentId, QString(), settingsKey());
}

void LauncherAspect::addToLayoutImpl(Layout &builder)
{
    auto comboBox = new QComboBox;

    const auto showCurrentLauncher = [this, comboBox] {
        const int index = Utils::indexOf(m_launchers, Utils::equal(&Launcher::id, m_currentId));
        const QSignalBlocker blocker(comboBox);
        comboBox->setCurrentIndex(index);
        comboBox->setToolTip(index < 0 ? QString() : m_launchers[index].command.toUserOutput());
    };
    const auto showLaunchers = [this, comboBox, showCurrentLauncher] {
        {
            const QSignalBlocker blocker(comboBox);
            comboBox->clear();
            for (const Launcher &launcher : std::as_const(m_launchers)) {
                comboBox->addItem(launcher.displayName);
                comboBox->setItemData(comboBox->count() - 1,
                                      launcher.command.toUserOutput(),
                                      Qt::ToolTipRole);
            }
        }
        showCurrentLauncher();
    };
    showLaunchers();

    connect(comboBox, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index < 0)
            return;
        QTC_ASSERT(index < m_launchers.size(), return);
        setCurrentLauncherId(m_launchers[index].id);
    });
    connect(this, &LauncherAspect::launchersChanged, comboBox, showLaunchers);
    connect(this, &LauncherAspect::currentLauncherChanged, comboBox, showCurrentLauncher);

    builder.addItems({Tr::tr("Launcher:"), comboBox});
}

void LauncherAspect::setCurrentLauncherId(const QString &id)
{
    if (id == m_currentId)
        return;
    m_currentId = id;
    emit currentLauncherChanged(QPrivateSignal());
    emit changed();
}

/*
    Makes an offered launcher the current one if the current one is not
    offered: the default launcher, or else the first one.
*/
void LauncherAspect::ensureOfferedCurrentLauncher()
{
    if (m_launchers.isEmpty())
        return;
    const auto isOffered = [this](const QString &id) {
        return Utils::anyOf(m_launchers, Utils::equal(&Launcher::id, id));
    };
    if (isOffered(m_currentId))
        return;
    setCurrentLauncherId(isOffered(m_defaultId) ? m_defaultId : m_launchers.first().id);
}

/*!
    \class ProjectExplorer::X11ForwardingAspect
    \inmodule QtCreator

    \brief The X11ForwardingAspect class lets a user specify a display
     for a remotely running X11 client.
*/

X11ForwardingAspect::X11ForwardingAspect(AspectContainer *container)
    : StringAspect(container)
{
    setDisplayStyle(LineEditDisplay);
    setId("X11ForwardingAspect");
    setSettingsKey("RunConfiguration.X11Forwarding");
    makeCheckable(
        CheckBoxPlacement::Left, Tr::tr("Use X11 forwarding:"), "RunConfiguration.UseX11Forwarding");
    setPlaceHolderText(hostX11Display());
    setLabelText("DISPLAY=");
    setToolTip(Tr::tr("Overrides the X11 display set in the device settings, "
                      "also when unchecked."));

    addDataExtractor(this, &X11ForwardingAspect::display, &Data::display);
}

QString X11ForwardingAspect::display() const
{
    if (!isChecked())
        return {};

    // Follow the current session unless the user pinned a display.
    const QString display = value().isEmpty() ? hostX11Display() : value();
    return macroExpander()->expand(display);
}


/*!
    \class ProjectExplorer::SymbolFileAspect
    \inmodule QtCreator

    \brief The SymbolFileAspect class lets a user specify a symbol file
     for debugging.
*/


SymbolFileAspect::SymbolFileAspect(AspectContainer *container)
    : FilePathAspect(container)
{}

MainScriptAspect::MainScriptAspect(AspectContainer *container)
    : FilePathAspect(container)
{}

#ifdef WITH_TESTS
namespace Internal {

class RunConfigurationAspectsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testTerminalAspectUpdatesEveryCheckBox();
    void testLauncherAspectUpdatesEveryComboBox();
    void testLauncherAspectPicksOfferedLauncherWithoutWidget();
    void testWorkingDirectoryAspectUpdatesEveryChooser();
    void testWorkingDirectoryAspectKeepsAClearedChooserEmpty();
    void testArgumentsAspectUpdatesEveryEditor();
    void testArgumentsAspectSwitchesEveryWidgetToMultiLine();
};

static std::unique_ptr<QWidget> createWidget(BaseAspect &aspect)
{
    return std::unique_ptr<QWidget>(Column{aspect}.emerge());
}

void RunConfigurationAspectsTest::testTerminalAspectUpdatesEveryCheckBox()
{
    TerminalAspect aspect;
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QCheckBox *firstBox = first->findChild<QCheckBox *>();
    QCheckBox *secondBox = second->findChild<QCheckBox *>();
    QVERIFY(firstBox);
    QVERIFY(secondBox);

    aspect.setVariantValue(true);
    QVERIFY(firstBox->isChecked());
    QVERIFY(secondBox->isChecked());

    firstBox->click();
    QVERIFY(!aspect.useTerminal());
    QVERIFY(!secondBox->isChecked());
}

static Launcher testLauncher(const QString &id)
{
    Launcher launcher;
    launcher.id = id;
    launcher.displayName = id;
    launcher.command = FilePath::fromString("/launchers/" + id);
    return launcher;
}

void RunConfigurationAspectsTest::testLauncherAspectUpdatesEveryComboBox()
{
    LauncherAspect aspect;
    aspect.updateLaunchers({testLauncher("a"), testLauncher("b"), testLauncher("c")});
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QComboBox *firstBox = first->findChild<QComboBox *>();
    QComboBox *secondBox = second->findChild<QComboBox *>();
    QVERIFY(firstBox);
    QVERIFY(secondBox);
    QCOMPARE(firstBox->count(), 3);
    QCOMPARE(secondBox->count(), 3);

    aspect.setCurrentLauncher(testLauncher("b"));
    QCOMPARE(firstBox->currentIndex(), 1);
    QCOMPARE(secondBox->currentIndex(), 1);

    firstBox->setCurrentIndex(2);
    QCOMPARE(aspect.currentLauncher().id, "c");
    QCOMPARE(secondBox->currentIndex(), 2);

    aspect.updateLaunchers({testLauncher("c"), testLauncher("d")});
    QCOMPARE(firstBox->count(), 2);
    QCOMPARE(secondBox->count(), 2);
    QCOMPARE(firstBox->currentIndex(), 0);
    QCOMPARE(secondBox->currentIndex(), 0);
}

void RunConfigurationAspectsTest::testLauncherAspectPicksOfferedLauncherWithoutWidget()
{
    LauncherAspect aspect;
    aspect.setDefaultLauncher(testLauncher("b"));
    aspect.updateLaunchers({testLauncher("a"), testLauncher("b"), testLauncher("c")});
    QCOMPARE(aspect.currentLauncher().id, "b");

    aspect.updateLaunchers({testLauncher("a"), testLauncher("c")});
    QCOMPARE(aspect.currentLauncher().id, "a");
}

void RunConfigurationAspectsTest::testWorkingDirectoryAspectUpdatesEveryChooser()
{
    WorkingDirectoryAspect aspect;
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    PathChooser *firstChooser = first->findChild<PathChooser *>();
    PathChooser *secondChooser = second->findChild<PathChooser *>();
    QVERIFY(firstChooser);
    QVERIFY(secondChooser);

    aspect.setWorkingDirectory(FilePath::fromString("/set/directory"));
    QCOMPARE(firstChooser->unexpandedFilePath(), FilePath::fromString("/set/directory"));
    QCOMPARE(secondChooser->unexpandedFilePath(), FilePath::fromString("/set/directory"));

    firstChooser->setFilePath(FilePath::fromString("/typed/directory"));
    QCOMPARE(aspect.unexpandedWorkingDirectory(), FilePath::fromString("/typed/directory"));
    QCOMPARE(secondChooser->unexpandedFilePath(), FilePath::fromString("/typed/directory"));

    Store map;
    map.insert(aspect.settingsKey(), "/stored/directory");
    static_cast<BaseAspect &>(aspect).fromMap(map);
    QCOMPARE(firstChooser->unexpandedFilePath(), FilePath::fromString("/stored/directory"));
    QCOMPARE(secondChooser->unexpandedFilePath(), FilePath::fromString("/stored/directory"));
}

void RunConfigurationAspectsTest::testWorkingDirectoryAspectKeepsAClearedChooserEmpty()
{
    WorkingDirectoryAspect aspect;
    aspect.setDefaultWorkingDirectory(FilePath::fromString("/default/directory"));
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    PathChooser *firstChooser = first->findChild<PathChooser *>();
    PathChooser *secondChooser = second->findChild<PathChooser *>();
    QVERIFY(firstChooser);
    QVERIFY(secondChooser);
    QCOMPARE(firstChooser->unexpandedFilePath(), FilePath::fromString("/default/directory"));

    firstChooser->setFilePath({});

    QVERIFY(firstChooser->unexpandedFilePath().isEmpty());
    QVERIFY(secondChooser->unexpandedFilePath().isEmpty());
}

void RunConfigurationAspectsTest::testArgumentsAspectUpdatesEveryEditor()
{
    ArgumentsAspect aspect;
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    FancyLineEdit *firstEdit = first->findChild<FancyLineEdit *>();
    FancyLineEdit *secondEdit = second->findChild<FancyLineEdit *>();
    QVERIFY(firstEdit);
    QVERIFY(secondEdit);

    aspect.setArguments("--set");
    QCOMPARE(firstEdit->text(), "--set");
    QCOMPARE(secondEdit->text(), "--set");

    firstEdit->setText("--typed");
    QCOMPARE(aspect.unexpandedArguments(), "--typed");
    QCOMPARE(secondEdit->text(), "--typed");
}

// ExpandButton has no Q_OBJECT.
static QToolButton *multiLineButton(const std::unique_ptr<QWidget> &widget)
{
    return Utils::findOrDefault(widget->findChildren<QToolButton *>(), [](QToolButton *button) {
        return button->toolTip() == Tr::tr("Toggle multi-line mode.");
    });
}

void RunConfigurationAspectsTest::testArgumentsAspectSwitchesEveryWidgetToMultiLine()
{
    ArgumentsAspect aspect;
    aspect.setArguments("--kept");
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QToolButton *firstButton = multiLineButton(first);
    QVERIFY(firstButton);
    QVERIFY(second->findChild<FancyLineEdit *>());

    firstButton->click();

    for (const std::unique_ptr<QWidget> *widget : {&first, &second}) {
        QVERIFY(!(*widget)->findChild<FancyLineEdit *>());
        QPlainTextEdit *editor = (*widget)->findChild<QPlainTextEdit *>();
        QVERIFY(editor);
        QCOMPARE(editor->toPlainText(), "--kept");
        QVERIFY(multiLineButton(*widget));
        QVERIFY(multiLineButton(*widget)->isChecked());
    }
}

QObject *createRunConfigurationAspectsTest()
{
    return new RunConfigurationAspectsTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // namespace ProjectExplorer

#ifdef WITH_TESTS
#include <runconfigurationaspects.moc>
#endif
