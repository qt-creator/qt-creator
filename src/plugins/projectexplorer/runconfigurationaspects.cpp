// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "runconfigurationaspects.h"

#include "devicesupport/devicemanager.h"
#include "devicesupport/idevice.h"
#include "environmentaspect.h"
#include "kitaspects.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"
#include "target.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/detailsbutton.h>
#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QPushButton>

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
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &TerminalAspect::calculateUseTerminal);
}

/*!
    \reimp
*/
void TerminalAspect::addToLayout(LayoutItem &parent)
{
    QTC_CHECK(!m_checkBox);
    m_checkBox = createSubWidget<QCheckBox>(Tr::tr("Run in terminal"));
    m_checkBox->setChecked(m_useTerminal);
    m_checkBox->setEnabled(isEnabled());
    parent.addItems({empty(), m_checkBox.data()});
    connect(m_checkBox.data(), &QAbstractButton::clicked, this, [this] {
        m_userSet = true;
        m_useTerminal = m_checkBox->isChecked();
        emit changed();
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

    if (m_checkBox)
        m_checkBox->setChecked(m_useTerminal);
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
    switch (ProjectExplorerPlugin::projectExplorerSettings().terminalMode) {
    case TerminalMode::On: useTerminal = true; break;
    case TerminalMode::Off: useTerminal = false; break;
    default: useTerminal = m_useTerminalHint;
    }
    if (m_useTerminal != useTerminal) {
        m_useTerminal = useTerminal;
        emit changed();
    }
    if (m_checkBox)
        m_checkBox->setChecked(m_useTerminal);
}

/*!
    Returns whether a separate terminal should be used.
*/
bool TerminalAspect::useTerminal() const
{
    return m_useTerminal && isEnabled();
}

/*!
    Sets the initial value to \a hint.
*/
void TerminalAspect::setUseTerminalHint(bool hint)
{
    m_useTerminalHint = hint;
    calculateUseTerminal();
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
    setId("WorkingDirectoryAspect");
    setSettingsKey("RunConfiguration.WorkingDirectory");
}

void WorkingDirectoryAspect::setMacroExpander(const MacroExpander *expander)
{
    m_macroExpander = expander;
}

void WorkingDirectoryAspect::setEnvironment(EnvironmentAspect *envAspect)
{
    m_envAspect = envAspect;
}

/*!
    \reimp
*/
void WorkingDirectoryAspect::addToLayout(LayoutItem &builder)
{
    QTC_CHECK(!m_chooser);
    m_chooser = new PathChooser;
    if (QTC_GUARD(m_macroExpander))
        m_chooser->setMacroExpander(m_macroExpander);
    m_chooser->setHistoryCompleter(settingsKey());
    m_chooser->setExpectedKind(Utils::PathChooser::Directory);
    m_chooser->setPromptDialogTitle(Tr::tr("Select Working Directory"));
    m_chooser->setBaseDirectory(m_defaultWorkingDirectory);
    m_chooser->setFilePath(m_workingDirectory.isEmpty() ? m_defaultWorkingDirectory : m_workingDirectory);
    connect(m_chooser.data(), &PathChooser::textChanged, this, [this] {
        m_workingDirectory = m_chooser->rawFilePath();
        m_resetButton->setEnabled(m_workingDirectory != m_defaultWorkingDirectory);
    });

    m_resetButton = new QToolButton;
    m_resetButton->setToolTip(Tr::tr("Reset to Default"));
    m_resetButton->setIcon(Utils::Icons::RESET.icon());
    connect(m_resetButton.data(), &QAbstractButton::clicked, this, &WorkingDirectoryAspect::resetPath);
    m_resetButton->setEnabled(m_workingDirectory != m_defaultWorkingDirectory);

    if (m_envAspect) {
        connect(m_envAspect, &EnvironmentAspect::environmentChanged, m_chooser.data(), [this] {
            m_chooser->setEnvironment(m_envAspect->environment());
        });
        m_chooser->setEnvironment(m_envAspect->environment());
    }

    builder.addItems({Tr::tr("Working directory:"), m_chooser.data(), m_resetButton.data()});
}

void WorkingDirectoryAspect::resetPath()
{
    m_chooser->setFilePath(m_defaultWorkingDirectory);
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

    if (m_chooser)
        m_chooser->setFilePath(m_workingDirectory.isEmpty() ? m_defaultWorkingDirectory : m_workingDirectory);
}

/*!
    \reimp
*/
void WorkingDirectoryAspect::toMap(Store &data) const
{
    const QString wd = m_workingDirectory == m_defaultWorkingDirectory
        ? QString() : m_workingDirectory.toString();
    saveToMap(data, wd, QString(), settingsKey());
    saveToMap(data, m_defaultWorkingDirectory.toString(), QString(), settingsKey() + ".default");
}

/*!
    Returns the selected directory.

    Macros in the value are expanded using \a expander.
*/
FilePath WorkingDirectoryAspect::workingDirectory() const
{
    const Environment env = m_envAspect ? m_envAspect->environment()
                                        : Environment::systemEnvironment();
    QString workingDir = m_workingDirectory.path();
    if (m_macroExpander)
        workingDir = m_macroExpander->expandProcessArgs(workingDir);

    QString res = workingDir.isEmpty() ? QString() : QDir::cleanPath(env.expandVariables(workingDir));

    return m_workingDirectory.withNewPath(res);
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
    Sets the default value to \a defaultWorkingDir.
*/
void WorkingDirectoryAspect::setDefaultWorkingDirectory(const FilePath &defaultWorkingDir)
{
    if (defaultWorkingDir == m_defaultWorkingDirectory)
        return;

    Utils::FilePath oldDefaultDir = m_defaultWorkingDirectory;
    m_defaultWorkingDirectory = defaultWorkingDir;
    if (m_chooser)
        m_chooser->setBaseDirectory(m_defaultWorkingDirectory);

    if (m_workingDirectory.isEmpty() || m_workingDirectory == oldDefaultDir) {
        if (m_chooser)
            m_chooser->setFilePath(m_defaultWorkingDirectory);
        m_workingDirectory = defaultWorkingDir;
    }
}

/*!
    \internal
*/
PathChooser *WorkingDirectoryAspect::pathChooser() const
{
    return m_chooser;
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
    setId("ArgumentsAspect");
    setSettingsKey("RunConfiguration.Arguments");

    addDataExtractor(this, &ArgumentsAspect::arguments, &Data::arguments);

    m_labelText = Tr::tr("Command line arguments:");
}

void ArgumentsAspect::setMacroExpander(const MacroExpander *expander)
{
    m_macroExpander = expander;
}

/*!
    Returns the main value of this aspect.

    Macros in the value are expanded using \a expander.
*/
QString ArgumentsAspect::arguments() const
{
    QTC_ASSERT(m_macroExpander, return m_arguments);
    if (m_currentlyExpanding)
        return m_arguments;

    m_currentlyExpanding = true;
    const QString expanded = m_macroExpander->expandProcessArgs(m_arguments);
    m_currentlyExpanding = false;
    return expanded;
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
    if (m_chooser && m_chooser->text() != arguments)
        m_chooser->setText(arguments);
    if (m_multiLineChooser && m_multiLineChooser->toPlainText() != arguments)
        m_multiLineChooser->setPlainText(arguments);
}

/*!
    Sets the displayed label text to \a labelText.
*/
void ArgumentsAspect::setLabelText(const QString &labelText)
{
    m_labelText = labelText;
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
    if (args.typeId() == QVariant::StringList)
        m_arguments = ProcessArgs::joinArgs(args.toStringList(), OsTypeLinux);
    else
        m_arguments = args.toString();

    m_multiLine = map.value(settingsKey() + ".multi", false).toBool();

    if (m_multiLineButton)
        m_multiLineButton->setChecked(m_multiLine);
    if (!m_multiLine && m_chooser)
        m_chooser->setText(m_arguments);
    if (m_multiLine && m_multiLineChooser)
        m_multiLineChooser->setPlainText(m_arguments);
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
QWidget *ArgumentsAspect::setupChooser()
{
    if (m_multiLine) {
        if (!m_multiLineChooser) {
            m_multiLineChooser = new QPlainTextEdit;
            connect(m_multiLineChooser.data(), &QPlainTextEdit::textChanged,
                    this, [this] { setArguments(m_multiLineChooser->toPlainText()); });
        }
        m_multiLineChooser->setPlainText(m_arguments);
        return m_multiLineChooser.data();
    }
    if (!m_chooser) {
        m_chooser = new FancyLineEdit;
        m_chooser->setHistoryCompleter(settingsKey());
        connect(m_chooser.data(), &QLineEdit::textChanged, this, &ArgumentsAspect::setArguments);
    }
    m_chooser->setText(m_arguments);
    return m_chooser.data();
}

/*!
    \reimp
*/
void ArgumentsAspect::addToLayout(LayoutItem &builder)
{
    QTC_CHECK(!m_chooser && !m_multiLineChooser && !m_multiLineButton);

    const auto container = new QWidget;
    const auto containerLayout = new QHBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->addWidget(setupChooser());
    m_multiLineButton = new ExpandButton;
    m_multiLineButton->setToolTip(Tr::tr("Toggle multi-line mode."));
    m_multiLineButton->setChecked(m_multiLine);
    connect(m_multiLineButton, &QCheckBox::clicked, this, [this](bool checked) {
        if (m_multiLine == checked)
            return;
        m_multiLine = checked;
        setupChooser();
        QWidget *oldWidget = nullptr;
        QWidget *newWidget = nullptr;
        if (m_multiLine) {
            oldWidget = m_chooser.data();
            newWidget = m_multiLineChooser.data();
        } else {
            oldWidget = m_multiLineChooser.data();
            newWidget = m_chooser.data();
        }
        QTC_ASSERT(!oldWidget == !newWidget, return);
        if (oldWidget) {
            QTC_ASSERT(oldWidget->parentWidget()->layout(), return);
            oldWidget->parentWidget()->layout()->replaceWidget(oldWidget, newWidget);
            delete oldWidget;
        }
    });
    containerLayout->addWidget(m_multiLineButton);
    containerLayout->setAlignment(m_multiLineButton, Qt::AlignTop);

    if (m_resetter) {
        m_resetButton = new QToolButton;
        m_resetButton->setToolTip(Tr::tr("Reset to Default"));
        m_resetButton->setIcon(Icons::RESET.icon());
        connect(m_resetButton.data(), &QAbstractButton::clicked,
                this, &ArgumentsAspect::resetArguments);
        containerLayout->addWidget(m_resetButton);
        containerLayout->setAlignment(m_resetButton, Qt::AlignTop);
    }

    builder.addItems({m_labelText, container});
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

static IDevice::ConstPtr executionDevice(Target *target,
                                         ExecutableAspect::ExecutionDeviceSelector selector)
{
    if (target) {
        if (selector == ExecutableAspect::RunDevice)
            return DeviceKitAspect::device(target->kit());
        if (selector == ExecutableAspect::BuildDevice)
            return BuildDeviceKitAspect::device(target->kit());
    }
    return DeviceManager::defaultDesktopDevice();
}

ExecutableAspect::~ExecutableAspect()
{
    delete m_alternativeExecutable;
    m_alternativeExecutable = nullptr;
}

void ExecutableAspect::setDeviceSelector(Target *target, ExecutionDeviceSelector selector)
{
    m_target = target;
    m_selector = selector;

    const IDevice::ConstPtr dev = executionDevice(m_target, m_selector);
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
void ExecutableAspect::setExpectedKind(const PathChooser::Kind expectedKind)
{
    m_executable.setExpectedKind(expectedKind);
    if (m_alternativeExecutable)
        m_alternativeExecutable->setExpectedKind(expectedKind);
}

/*!
   Sets the environment in which paths will be searched when the expected kind
   of paths is chosen as PathChooser::Command or PathChooser::ExistingCommand
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
    m_executable.setReadOnly(readOnly);
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

    if (const IDevice::ConstPtr dev = executionDevice(m_target, m_selector))
        exe = dev->rootPath().withNewMappedPath(exe);

    return exe;
}

/*!
    \reimp
*/
void ExecutableAspect::addToLayout(LayoutItem &builder)
{
    builder.addItem(m_executable);
    if (m_alternativeExecutable)
        builder.addItems({br, m_alternativeExecutable});
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
    if (HostOsInfo::isMacHost()) {
        setLabel(Tr::tr("Add build library search path to DYLD_LIBRARY_PATH and DYLD_FRAMEWORK_PATH"),
                 LabelPlacement::AtCheckBox);
    } else if (HostOsInfo::isWindowsHost()) {
        setLabel(Tr::tr("Add build library search path to PATH"), LabelPlacement::AtCheckBox);
    } else {
        setLabel(Tr::tr("Add build library search path to LD_LIBRARY_PATH"),
                 LabelPlacement::AtCheckBox);
    }
    setValue(ProjectExplorerPlugin::projectExplorerSettings().addLibraryPathsToRunEnv);
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
}

/*!
    \class ProjectExplorer::RunAsRootAspect
    \inmodule QtCreator

    \brief The RunAsRootAspect class lets a user specify whether the
    application should run with root permissions.
*/

RunAsRootAspect::RunAsRootAspect(AspectContainer *container)
    : BoolAspect(container)
{
    setId("RunAsRoot");
    setSettingsKey("RunConfiguration.RunAsRoot");
    setLabel(Tr::tr("Run as root user"), LabelPlacement::AtCheckBox);
}

Interpreter::Interpreter()
    : id(QUuid::createUuid().toString())
{}

Interpreter::Interpreter(const QString &_id,
                         const QString &_name,
                         const FilePath &_command,
                         bool _autoDetected)
    : id(_id)
    , name(_name)
    , command(_command)
    , autoDetected(_autoDetected)
{}

/*!
    \class ProjectExplorer::InterpreterAspect
    \inmodule QtCreator

    \brief The InterpreterAspect class lets a user specify an interpreter
    to use with files or projects using an interpreted language.
*/

InterpreterAspect::InterpreterAspect(AspectContainer *container)
    : BaseAspect(container)
{
    addDataExtractor(this, &InterpreterAspect::currentInterpreter, &Data::interpreter);
}

Interpreter InterpreterAspect::currentInterpreter() const
{
    return Utils::findOrDefault(m_interpreters, Utils::equal(&Interpreter::id, m_currentId));
}

void InterpreterAspect::updateInterpreters(const QList<Interpreter> &interpreters)
{
    if (m_interpreters == interpreters)
        return;
    m_interpreters = interpreters;
    if (m_comboBox)
        updateComboBox();
}

void InterpreterAspect::setDefaultInterpreter(const Interpreter &interpreter)
{
    if (m_defaultId == interpreter.id)
        return;
    m_defaultId = interpreter.id;
    if (m_currentId.isEmpty())
        setCurrentInterpreter(interpreter);
}

void InterpreterAspect::setCurrentInterpreter(const Interpreter &interpreter)
{
    if (m_comboBox) {
        const int index = m_interpreters.indexOf(interpreter);
        if (index < 0 || index >= m_comboBox->count())
            return;
        m_comboBox->setCurrentIndex(index);
    } else {
        setCurrentInterpreterId(interpreter.id);
    }
}

void InterpreterAspect::fromMap(const Store &map)
{
    setCurrentInterpreterId(map.value(settingsKey(), m_defaultId).toString());
}

void InterpreterAspect::toMap(Store &map) const
{
    if (m_currentId != m_defaultId)
        saveToMap(map, m_currentId, QString(), settingsKey());
}

void InterpreterAspect::addToLayout(LayoutItem &builder)
{
    if (QTC_GUARD(m_comboBox.isNull()))
        m_comboBox = new QComboBox;

    updateComboBox();
    connect(m_comboBox, &QComboBox::currentIndexChanged,
            this, &InterpreterAspect::updateCurrentInterpreter);

    auto manageButton = new QPushButton(Tr::tr("Manage..."));
    connect(manageButton, &QPushButton::clicked, this, [this] {
        Core::ICore::showOptionsDialog(m_settingsDialogId);
    });

    builder.addItems({Tr::tr("Interpreter:"), m_comboBox.data(), manageButton});
}

void InterpreterAspect::setCurrentInterpreterId(const QString &id)
{
    if (id == m_currentId)
        return;
    m_currentId = id;
    emit changed();
}

void InterpreterAspect::updateCurrentInterpreter()
{
    const int index = m_comboBox->currentIndex();
    if (index < 0)
        return;
    QTC_ASSERT(index < m_interpreters.size(), return);
    m_comboBox->setToolTip(m_interpreters[index].command.toUserOutput());
    setCurrentInterpreterId(m_interpreters[index].id);
}

void InterpreterAspect::updateComboBox()
{
    int currentIndex = -1;
    int defaultIndex = -1;
    m_comboBox->clear();
    for (const Interpreter &interpreter : std::as_const(m_interpreters)) {
        int index = m_comboBox->count();
        m_comboBox->addItem(interpreter.name);
        m_comboBox->setItemData(index, interpreter.command.toUserOutput(), Qt::ToolTipRole);
        if (interpreter.id == m_currentId)
            currentIndex = index;
        if (interpreter.id == m_defaultId)
            defaultIndex = index;
    }
    if (currentIndex >= 0)
        m_comboBox->setCurrentIndex(currentIndex);
    else if (defaultIndex >= 0)
        m_comboBox->setCurrentIndex(defaultIndex);
    updateCurrentInterpreter();
}

/*!
    \class ProjectExplorer::X11ForwardingAspect
    \inmodule QtCreator

    \brief The X11ForwardingAspect class lets a user specify a display
     for a remotely running X11 client.
*/

static QString defaultDisplay()
{
    return qtcEnvironmentVariable("DISPLAY");
}

X11ForwardingAspect::X11ForwardingAspect(AspectContainer *container)
    : StringAspect(container)
{
    setLabelText(Tr::tr("X11 Forwarding:"));
    setDisplayStyle(LineEditDisplay);
    setId("X11ForwardingAspect");
    setSettingsKey("RunConfiguration.X11Forwarding");
    makeCheckable(CheckBoxPlacement::Right, Tr::tr("Forward to local display"),
                  "RunConfiguration.UseX11Forwarding");
    setValue(defaultDisplay());

    addDataExtractor(this, &X11ForwardingAspect::display, &Data::display);
}

void X11ForwardingAspect::setMacroExpander(const MacroExpander *expander)
{
   m_macroExpander = expander;
}

QString X11ForwardingAspect::display() const
{
    QTC_ASSERT(m_macroExpander, return value());
    return !isChecked() ? QString() : m_macroExpander->expandProcessArgs(value());
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

} // namespace ProjectExplorer
