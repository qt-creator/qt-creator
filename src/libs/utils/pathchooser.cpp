// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pathchooser.h"

#include "async.h"
#include "commandline.h"
#include "environment.h"
#include "fileutils.h"
#include "guard.h"
#include "hostosinfo.h"
#include "macroexpander.h"
#include "optionpushbutton.h"
#include "process.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QFileDialog>
#include <QFuture>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QStandardPaths>

/*!
    \class Utils::PathChooser
    \inmodule QtCreator

    \brief The PathChooser class is a control that lets the user choose a path.
    The control consist of a QLineEdit and a "Browse" button, and is optionally
    able to perform variable substitution.

    This class has some validation logic for embedding into QWizardPage.
*/

/*!
    \enum Utils::PathChooser::Kind
    \inmodule QtCreator

    The Kind enum describes the kind of path a PathChooser considers valid.

    \value ExistingDirectory An existing directory
    \value Directory A directory that does not need to exist
    \value File An existing file
    \value SaveFile A file that does not need to exist
    \value ExistingCommand An executable file that must exist at the time of selection
    \value Command An executable file that may or may not exist at the time of selection (e.g. result of a build)
    \value Any No restriction on the selected path

    \sa setExpectedKind(), expectedKind()
*/

namespace Utils {

static FilePath appBundleExpandedPath(const FilePath &path)
{
    if (path.osType() == OsTypeMac && path.endsWith(".app")) {
        // possibly expand to Foo.app/Contents/MacOS/Foo
        if (path.isDir()) {
            const FilePath exePath = path / "Contents/MacOS" / path.completeBaseName();
            if (exePath.exists())
                return exePath;
        }
    }
    return path;
}

PathChooser::AboutToShowContextMenuHandler PathChooser::s_aboutToShowContextMenuHandler;

// ------------------ BinaryVersionToolTipEventFilter
// Event filter to be installed on a lineedit used for entering
// executables, taking the arguments to print the version ('--version').
// On a tooltip event, the version is obtained by running the binary and
// setting its stdout as tooltip.

class BinaryVersionToolTipEventFilter : public QObject
{
public:
    explicit BinaryVersionToolTipEventFilter(QLineEdit *le);

    bool eventFilter(QObject *, QEvent *) override;

    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList &arguments) { m_arguments = arguments; }

    static QString toolVersion(const CommandLine &cmd);

private:
    // Extension point for concatenating existing tooltips.
    virtual QString defaultToolTip() const  { return QString(); }

    QStringList m_arguments;
};

BinaryVersionToolTipEventFilter::BinaryVersionToolTipEventFilter(QLineEdit *le) :
    QObject(le)
{
    le->installEventFilter(this);
}

bool BinaryVersionToolTipEventFilter::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() != QEvent::ToolTip)
        return false;
    auto le = qobject_cast<QLineEdit *>(o);
    QTC_ASSERT(le, return false);

    const QString binary = le->text();
    if (!binary.isEmpty()) {
        const QString version = BinaryVersionToolTipEventFilter::toolVersion(
                    CommandLine(FilePath::fromString(QDir::cleanPath(binary)), m_arguments));
        if (!version.isEmpty()) {
            // Concatenate tooltips.
            QString tooltip = "<html><head/><body>";
            const QString defaultValue = defaultToolTip();
            if (!defaultValue.isEmpty()) {
                tooltip += "<p>";
                tooltip += defaultValue;
                tooltip += "</p>";
            }
            tooltip += "<pre>";
            tooltip += version;
            tooltip += "</pre><body></html>";
            le->setToolTip(tooltip);
        }
    }
    return false;
}

QString BinaryVersionToolTipEventFilter::toolVersion(const CommandLine &cmd)
{
    if (cmd.executable().isEmpty())
        return QString();
    Process proc;
    proc.setTimeoutS(1);
    proc.setCommand(cmd);
    proc.runBlocking();
    if (proc.result() != ProcessResult::FinishedWithSuccess)
        return QString();
    return proc.allOutput();
}

// Extends BinaryVersionToolTipEventFilter to prepend the existing pathchooser
// tooltip to display the full path.
class PathChooserBinaryVersionToolTipEventFilter : public BinaryVersionToolTipEventFilter
{
public:
    explicit PathChooserBinaryVersionToolTipEventFilter(PathChooser *pe) :
        BinaryVersionToolTipEventFilter(pe->lineEdit()), m_pathChooser(pe) {}

private:
    QString defaultToolTip() const override { return m_pathChooser->errorMessage(); }

    const PathChooser *m_pathChooser = nullptr;
};

// ------------------ PathChooserPrivate

class PathChooserPrivate
{
public:
    PathChooserPrivate();

    FilePath expandedPath(const FilePath &path) const;

    QHBoxLayout *m_hLayout = nullptr;
    FancyLineEdit *m_lineEdit = nullptr;

    PathChooser::Kind m_acceptingKind = PathChooser::ExistingDirectory;
    QString m_dialogTitleOverride;
    QString m_dialogFilter;
    FilePath m_initialBrowsePathOverride;
    QString m_defaultValue;
    FilePath m_baseDirectory;
    Environment m_environment;
    BinaryVersionToolTipEventFilter *m_binaryVersionToolTipEventFilter = nullptr;
    QList<QAbstractButton *> m_buttons;
    const MacroExpander *m_macroExpander = globalMacroExpander();
    std::function<void()> m_openTerminal;
    bool m_allowPathFromDevice = false;

    QMenu *m_contextMenu = nullptr;
    OptionPushButton *m_browseButton = nullptr;
    Guard m_callGuard;
};

PathChooserPrivate::PathChooserPrivate()
    : m_hLayout(new QHBoxLayout)
    , m_lineEdit(new FancyLineEdit)
{
}

FilePath PathChooserPrivate::expandedPath(const FilePath &input) const
{
    if (input.isEmpty())
        return {};

    FilePath path = input;

    Environment env = m_environment.appliedToEnvironment(path.deviceEnvironment());
    path = env.expandVariables(path);

    if (m_macroExpander)
        path = m_macroExpander->expand(path);

    if (path.isEmpty())
        return path;

    if (path.isAbsolutePath())
        return path;

    switch (m_acceptingKind) {
    case PathChooser::Command:
    case PathChooser::ExistingCommand: {
        const FilePath expanded = path.searchInPath({m_baseDirectory});
        return expanded.isEmpty() ? path : expanded;
    }
    case PathChooser::Any:
        break;
    case PathChooser::Directory:
    case PathChooser::ExistingDirectory:
    case PathChooser::File:
    case PathChooser::SaveFile:
        if (!m_baseDirectory.isEmpty()) {
            FilePath fp = m_baseDirectory.resolvePath(path.path()).absoluteFilePath();
            // FIXME bad hotfix for manually editing PathChooser (invalid paths, jumping cursor)
            // examples: have an absolute path and try to change the device letter by typing the new
            // letter and removing the original afterwards ends up in
            // D:\\dev\\project\\cD:\\dev\\build-project (before trying to remove the original)
            // as 'cD:\\dev\\build-project' is considered is handled as being relative
            // input = "cD:\\dev\build-project"; // prepended 'c' to change the device letter
            // m_baseDirectory = "D:\\dev\\project"
            if (!fp.needsDevice() && HostOsInfo::isWindowsHost() && fp.toString().count(':') > 1)
                return path;
            return fp;
        }
        break;
    }
    return path;
}

PathChooser::PathChooser(QWidget *parent) :
    QWidget(parent),
    d(new PathChooserPrivate)
{
    d->m_hLayout->setContentsMargins(0, 0, 0, 0);

    d->m_lineEdit->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(d->m_lineEdit,
            &FancyLineEdit::customContextMenuRequested,
            this,
            &PathChooser::contextMenuRequested);
    connect(d->m_lineEdit, &FancyLineEdit::validReturnPressed, this, &PathChooser::returnPressed);
    connect(d->m_lineEdit, &QLineEdit::textChanged, this, &PathChooser::rawPathChanged);
    connect(d->m_lineEdit, &FancyLineEdit::validChanged, this, &PathChooser::validChanged);
    connect(d->m_lineEdit, &QLineEdit::editingFinished, this, &PathChooser::editingFinished);
    connect(d->m_lineEdit, &QLineEdit::textChanged, this, &PathChooser::textChanged);

    d->m_lineEdit->setMinimumWidth(120);
    d->m_hLayout->addWidget(d->m_lineEdit);
    d->m_hLayout->setSizeConstraint(QLayout::SetMinimumSize);

    d->m_browseButton = new OptionPushButton;
    d->m_browseButton->setText(browseButtonLabel());
    connect(d->m_browseButton, &OptionPushButton::clicked, this, [this] { slotBrowse(false); });

    d->m_contextMenu = new QMenu(d->m_browseButton);
    d->m_contextMenu->addAction(Tr::tr("Local"), this, [this] { slotBrowse(false); });
    d->m_contextMenu->addAction(Tr::tr("Remote"), this, [this] { slotBrowse(true); });

    insertButton(d->m_buttons.count(), d->m_browseButton);

    setLayout(d->m_hLayout);
    setFocusProxy(d->m_lineEdit);
    setFocusPolicy(d->m_lineEdit->focusPolicy());

    d->m_lineEdit->setValidationFunction(defaultValidationFunction());
}

PathChooser::~PathChooser()
{
    // Since it is our focusProxy it can receive focus-out and emit the signal
    // even when the possible ancestor-receiver is in mid of its destruction.
    disconnect(d->m_lineEdit, &QLineEdit::editingFinished, this, &PathChooser::editingFinished);

    delete d;
}

void PathChooser::addButton(const QString &text, QObject *context, const std::function<void ()> &callback)
{
    insertButton(d->m_buttons.count(), text, context, callback);
}

void PathChooser::insertButton(int index, QAbstractButton *button)
{
    d->m_hLayout->insertWidget(index + 1 /*line edit*/, button);
    d->m_buttons.insert(index, button);
}

void PathChooser::insertButton(int index, const QString &text, QObject *context, const std::function<void ()> &callback)
{
    auto button = new QPushButton;
    button->setText(text);
    connect(button, &QAbstractButton::clicked, context, callback);
    insertButton(index, button);
}

QString PathChooser::browseButtonLabel()
{
    return HostOsInfo::isMacHost() ? Tr::tr("Choose...") : Tr::tr("Browse...");
}

QAbstractButton *PathChooser::buttonAtIndex(int index) const
{
    return d->m_buttons.at(index);
}

void PathChooser::setBaseDirectory(const FilePath &base)
{
    if (d->m_baseDirectory == base)
        return;
    d->m_baseDirectory = base;
    triggerChanged();
}

FilePath PathChooser::baseDirectory() const
{
    return d->m_baseDirectory;
}

void PathChooser::setEnvironment(const Environment &env)
{
    QString oldExpand = filePath().toString();
    d->m_environment = env;
    if (filePath().toString() != oldExpand) {
        triggerChanged();
        emit rawPathChanged();
    }
}

FilePath PathChooser::rawFilePath() const
{
    return FilePath::fromUserInput(d->m_lineEdit->text());
}

FilePath PathChooser::filePath() const
{
    return d->expandedPath(rawFilePath());
}

FilePath PathChooser::absoluteFilePath() const
{
    return d->m_baseDirectory.resolvePath(filePath());
}

void PathChooser::setPath(const QString &path)
{
    QTC_ASSERT(!d->m_callGuard.isLocked(), return);
    GuardLocker locker(d->m_callGuard);
    d->m_lineEdit->setTextKeepingActiveCursor(QDir::toNativeSeparators(path));
}

void PathChooser::setFilePath(const FilePath &fn)
{
    QTC_ASSERT(!d->m_callGuard.isLocked(), return);
    GuardLocker locker(d->m_callGuard);
    d->m_lineEdit->setTextKeepingActiveCursor(fn.toUserOutput());
}

bool PathChooser::isReadOnly() const
{
    return d->m_lineEdit->isReadOnly();
}

void PathChooser::setReadOnly(bool b)
{
    d->m_lineEdit->setReadOnly(b);
    d->m_lineEdit->setFrame(!b);
    const auto buttons = d->m_buttons;
    for (QAbstractButton *button : buttons)
        button->setVisible(!b);
}

void PathChooser::slotBrowse(bool remote)
{
    emit beforeBrowsing();

    FilePath predefined = filePath();

    if (!predefined.isEmpty() && !predefined.isDir()) {
        predefined = predefined.parentDir();
    }

    if ((predefined.isEmpty() || !predefined.isDir()) && !d->m_initialBrowsePathOverride.isEmpty()) {
        predefined = d->m_initialBrowsePathOverride;
        if (!predefined.isDir())
            predefined.clear();
    }

    remote = remote || filePath().needsDevice();

    // Prompt for a file/dir
    FilePath newPath;
    switch (d->m_acceptingKind) {
    case PathChooser::Directory:
    case PathChooser::ExistingDirectory:
        newPath = FileUtils::getExistingDirectory(this,
                                                  makeDialogTitle(Tr::tr("Choose Directory")),
                                                  predefined,
                                                  {},
                                                  d->m_allowPathFromDevice,
                                                  remote);
        break;
    case PathChooser::ExistingCommand:
    case PathChooser::Command:
        newPath = FileUtils::getOpenFilePath(this,
                                             makeDialogTitle(Tr::tr("Choose Executable")),
                                             predefined,
                                             d->m_dialogFilter,
                                             nullptr,
                                             {},
                                             d->m_allowPathFromDevice,
                                             remote);
        newPath = appBundleExpandedPath(newPath);
        break;
    case PathChooser::File: // fall through
        newPath = FileUtils::getOpenFilePath(this,
                                             makeDialogTitle(Tr::tr("Choose File")),
                                             predefined,
                                             d->m_dialogFilter,
                                             nullptr,
                                             {},
                                             d->m_allowPathFromDevice,
                                             remote);
        newPath = appBundleExpandedPath(newPath);
        break;
    case PathChooser::SaveFile:
        newPath = FileUtils::getSaveFilePath(this,
                                             makeDialogTitle(Tr::tr("Choose File")),
                                             predefined,
                                             d->m_dialogFilter,
                                             nullptr,
                                             {},
                                             remote);
        break;
    case PathChooser::Any: {
        newPath = FileUtils::getOpenFilePath(this,
                                             makeDialogTitle(Tr::tr("Choose File")),
                                             predefined,
                                             d->m_dialogFilter,
                                             nullptr,
                                             {},
                                             d->m_allowPathFromDevice,
                                             remote);
        break;
    }
    default:
        break;
    }

    // work around QTBUG-61004 / QTCREATORBUG-22906
    window()->raise();
    window()->activateWindow();

    // Delete trailing slashes unless it is "/" only.
    if (!newPath.isEmpty()) {
        if (newPath.endsWith("/") && newPath.path().size() > 1)
            newPath = newPath.withNewPath(newPath.path().chopped(1));
        setFilePath(newPath);
    }

    emit browsingFinished();
    triggerChanged();
}

void PathChooser::contextMenuRequested(const QPoint &pos)
{
    if (QMenu *menu = d->m_lineEdit->createStandardContextMenu()) {
        menu->setAttribute(Qt::WA_DeleteOnClose);

        if (s_aboutToShowContextMenuHandler)
            s_aboutToShowContextMenuHandler(this, menu);

        menu->popup(d->m_lineEdit->mapToGlobal(pos));
    }
}

bool PathChooser::isValid() const
{
    return d->m_lineEdit->isValid();
}

QString PathChooser::errorMessage() const
{
    return d->m_lineEdit->errorMessage();
}

void PathChooser::triggerChanged()
{
    d->m_lineEdit->validate();
}

void PathChooser::setAboutToShowContextMenuHandler(PathChooser::AboutToShowContextMenuHandler handler)
{
    s_aboutToShowContextMenuHandler = handler;
}

void PathChooser::setOpenTerminalHandler(const std::function<void ()> &openTerminal)
{
    d->m_openTerminal = openTerminal;
}

std::function<void()> PathChooser::openTerminalHandler() const
{
    return d->m_openTerminal;
}

void PathChooser::setDefaultValue(const QString &defaultValue)
{
    d->m_defaultValue = defaultValue;
    d->m_lineEdit->setPlaceholderText(defaultValue);
    d->m_lineEdit->validate();
}

void PathChooser::setPlaceholderText(const QString &placeholderText)
{
    d->m_lineEdit->setPlaceholderText(placeholderText);
}

void PathChooser::setToolTip(const QString &toolTip)
{
    d->m_lineEdit->setToolTip(toolTip);
}

static FancyLineEdit::AsyncValidationResult validatePath(FilePath filePath,
                                                         const QString &defaultValue,
                                                         PathChooser::Kind kind)
{
    if (filePath.isEmpty()) {
        if (!defaultValue.isEmpty()) {
            filePath = FilePath::fromUserInput(defaultValue);
        } else {
            return make_unexpected(Tr::tr("The path must not be empty."));
        }
    }

    // Check if existing
    switch (kind) {
    case PathChooser::ExistingDirectory:
        if (!filePath.exists()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" does not exist.").arg(filePath.toUserOutput()));
        }
        if (!filePath.isDir()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" is not a directory.").arg(filePath.toUserOutput()));
        }
        break;
    case PathChooser::File:
        if (!filePath.exists()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" does not exist.").arg(filePath.toUserOutput()));
        }
        if (!filePath.isFile()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" is not a file.").arg(filePath.toUserOutput()));
        }
        break;
    case PathChooser::SaveFile:
        if (!filePath.parentDir().exists()) {
            return make_unexpected(
                Tr::tr("The directory \"%1\" does not exist.").arg(filePath.toUserOutput()));
        }
        if (filePath.exists() && filePath.isDir()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" is not a file.").arg(filePath.toUserOutput()));
        }
        break;
    case PathChooser::ExistingCommand:
        if (!filePath.exists()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" does not exist.").arg(filePath.toUserOutput()));
        }
        if (!filePath.isExecutableFile()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" is not an executable file.").arg(filePath.toUserOutput()));
        }
        break;
    case PathChooser::Directory:
        if (filePath.exists() && !filePath.isDir()) {
            return make_unexpected(
                Tr::tr("The path \"%1\" is not a directory.").arg(filePath.toUserOutput()));
        }
        if (filePath.osType() == OsTypeWindows && !filePath.startsWithDriveLetter()
            && !filePath.startsWith("\\\\") && !filePath.startsWith("//")) {
            return make_unexpected(Tr::tr("Invalid path \"%1\".").arg(filePath.toUserOutput()));
        }
        break;
    case PathChooser::Command:
        if (filePath.exists() && !filePath.isExecutableFile()) {
            return make_unexpected(Tr::tr("Cannot execute \"%1\".").arg(filePath.toUserOutput()));
        }
        break;

    default:
        ;
    }

    return filePath.toUserOutput();
}

FancyLineEdit::AsyncValidationFunction PathChooser::defaultValidationFunction() const
{
    return [this](const QString &text) -> FancyLineEdit::AsyncValidationFuture {
        if (text.isEmpty()) {
            return QtFuture::makeReadyFuture((Utils::expected_str<QString>(
                make_unexpected(Tr::tr("The path must not be empty.")))));
        }

        const FilePath expanded = d->expandedPath(FilePath::fromUserInput(text));

        if (expanded.isEmpty()) {
            return QtFuture::makeReadyFuture((Utils::expected_str<QString>(
                make_unexpected(Tr::tr("The path \"%1\" expanded to an empty string.")
                                    .arg(expanded.toUserOutput())))));
        }

        return Utils::asyncRun(
            [expanded,
             defVal = d->m_defaultValue,
             kind = d->m_acceptingKind]() -> FancyLineEdit::AsyncValidationResult {
                return validatePath(expanded, defVal, kind);
            });
    };
}

void PathChooser::setValidationFunction(const FancyLineEdit::ValidationFunction &fn)
{
    d->m_lineEdit->setValidationFunction(fn);
}

QString PathChooser::label()
{
    return Tr::tr("Path:");
}

FilePath PathChooser::homePath()
{
    // Return 'users/<name>/Documents' on Windows, since Windows explorer
    // does not let people actually display the contents of their home
    // directory. Alternatively, create a QtCreator-specific directory?
    if (HostOsInfo::isWindowsHost())
        return FilePath::fromString(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    return FilePath::fromString(QDir::homePath());
}

/*!
    Sets the kind of path the PathChooser will consider valid to select
    to \a expected.

    \sa Utils::PathChooser::Kind, expectedKind()
*/

void PathChooser::setExpectedKind(Kind expected)
{
    if (d->m_acceptingKind == expected)
        return;
    d->m_acceptingKind = expected;
    d->m_lineEdit->validate();
}

/*!
    Returns the kind of path the PathChooser considers valid to select.

    \sa Utils::PathChooser::Kind, setExpectedKind()
*/

PathChooser::Kind PathChooser::expectedKind() const
{
    return d->m_acceptingKind;
}

void PathChooser::setPromptDialogTitle(const QString &title)
{
    d->m_dialogTitleOverride = title;
}

QString PathChooser::promptDialogTitle() const
{
    return d->m_dialogTitleOverride;
}

void PathChooser::setPromptDialogFilter(const QString &filter)
{
    d->m_dialogFilter = filter;
    d->m_lineEdit->validate();
}

QString PathChooser::promptDialogFilter() const
{
    return d->m_dialogFilter;
}

void PathChooser::setInitialBrowsePathBackup(const FilePath &path)
{
    d->m_initialBrowsePathOverride = path;
}

QString PathChooser::makeDialogTitle(const QString &title)
{
    if (d->m_dialogTitleOverride.isNull())
        return title;
    else
        return d->m_dialogTitleOverride;
}

FancyLineEdit *PathChooser::lineEdit() const
{
    // HACK: Make it work with HistoryCompleter.
    if (d->m_lineEdit->objectName().isEmpty())
        d->m_lineEdit->setObjectName(objectName() + "LineEdit");
    return d->m_lineEdit;
}

QString PathChooser::toolVersion(const CommandLine &cmd)
{
    return BinaryVersionToolTipEventFilter::toolVersion(cmd);
}

void PathChooser::installLineEditVersionToolTip(QLineEdit *le, const QStringList &arguments)
{
    auto ef = new BinaryVersionToolTipEventFilter(le);
    ef->setArguments(arguments);
}

void PathChooser::setHistoryCompleter(const Key &historyKey, bool restoreLastItemFromHistory)
{
    d->m_lineEdit->setHistoryCompleter(historyKey, restoreLastItemFromHistory);
}

void PathChooser::setMacroExpander(const MacroExpander *macroExpander)
{
    d->m_macroExpander = macroExpander;
}

QStringList PathChooser::commandVersionArguments() const
{
    return d->m_binaryVersionToolTipEventFilter ?
           d->m_binaryVersionToolTipEventFilter->arguments() :
           QStringList();
}

void PathChooser::setCommandVersionArguments(const QStringList &arguments)
{
    if (arguments.isEmpty()) {
        if (d->m_binaryVersionToolTipEventFilter) {
            delete d->m_binaryVersionToolTipEventFilter;
            d->m_binaryVersionToolTipEventFilter = nullptr;
        }
    } else {
        if (!d->m_binaryVersionToolTipEventFilter)
            d->m_binaryVersionToolTipEventFilter = new PathChooserBinaryVersionToolTipEventFilter(this);
        d->m_binaryVersionToolTipEventFilter->setArguments(arguments);
    }
}

void PathChooser::setAllowPathFromDevice(bool allow)
{
    d->m_allowPathFromDevice = allow;

    if (allow && FileUtils::hasNativeFileDialog())
        d->m_browseButton->setOptionalMenu(d->m_contextMenu);
    else
        d->m_browseButton->setOptionalMenu(nullptr);
}

bool PathChooser::allowPathFromDevice() const
{
    return d->m_allowPathFromDevice;
}

} // namespace Utils
