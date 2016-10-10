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

#include "pathchooser.h"

#include "environment.h"
#include "qtcassert.h"

#include "synchronousprocess.h"
#include "hostosinfo.h"
#include "theme/theme.h"

#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QStandardPaths>

/*!
    \class Utils::PathChooser

    \brief The PathChooser class is a control that lets the user choose a path,
    consisting of a QLineEdit and
    a "Browse" button.

    This class has some validation logic for embedding into QWizardPage.
*/

static QString appBundleExpandedPath(const QString &path)
{
    if (Utils::HostOsInfo::hostOs() == Utils::OsTypeMac && path.endsWith(QLatin1String(".app"))) {
        // possibly expand to Foo.app/Contents/MacOS/Foo
        QFileInfo info(path);
        if (info.isDir()) {
            QString exePath = path + QLatin1String("/Contents/MacOS/") + info.completeBaseName();
            if (QFileInfo(exePath).exists())
                return exePath;
        }
    }
    return path;
}

Utils::PathChooser::AboutToShowContextMenuHandler Utils::PathChooser::s_aboutToShowContextMenuHandler;

namespace Utils {

// ------------------ BinaryVersionToolTipEventFilter
// Event filter to be installed on a lineedit used for entering
// executables, taking the arguments to print the version ('--version').
// On a tooltip event, the version is obtained by running the binary and
// setting its stdout as tooltip.

class BinaryVersionToolTipEventFilter : public QObject
{
public:
    explicit BinaryVersionToolTipEventFilter(QLineEdit *le);

    virtual bool eventFilter(QObject *, QEvent *);

    QStringList arguments() const { return m_arguments; }
    void setArguments(const QStringList &arguments) { m_arguments = arguments; }

    static QString toolVersion(const QString &binary, const QStringList &arguments);

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
    QLineEdit *le = qobject_cast<QLineEdit *>(o);
    QTC_ASSERT(le, return false);

    const QString binary = le->text();
    if (!binary.isEmpty()) {
        const QString version = BinaryVersionToolTipEventFilter::toolVersion(QDir::cleanPath(binary), m_arguments);
        if (!version.isEmpty()) {
            // Concatenate tooltips.
            QString tooltip = QLatin1String("<html><head/><body>");
            const QString defaultValue = defaultToolTip();
            if (!defaultValue.isEmpty()) {
                tooltip += QLatin1String("<p>");
                tooltip += defaultValue;
                tooltip += QLatin1String("</p>");
            }
            tooltip += QLatin1String("<pre>");
            tooltip += version;
            tooltip += QLatin1String("</pre><body></html>");
            le->setToolTip(tooltip);
        }
    }
    return false;
}

QString BinaryVersionToolTipEventFilter::toolVersion(const QString &binary, const QStringList &arguments)
{
    if (binary.isEmpty())
        return QString();
    SynchronousProcess proc;
    proc.setTimeoutS(1);
    SynchronousProcessResponse response = proc.runBlocking(binary, arguments);
    if (response.result != SynchronousProcessResponse::Finished)
        return QString();
    return response.allOutput();
}

// Extends BinaryVersionToolTipEventFilter to prepend the existing pathchooser
// tooltip to display the full path.
class PathChooserBinaryVersionToolTipEventFilter : public BinaryVersionToolTipEventFilter
{
public:
    explicit PathChooserBinaryVersionToolTipEventFilter(PathChooser *pe) :
        BinaryVersionToolTipEventFilter(pe->lineEdit()), m_pathChooser(pe) {}

private:
    virtual QString defaultToolTip() const
        { return m_pathChooser->errorMessage(); }

    const PathChooser *m_pathChooser;
};

// ------------------ PathChooserPrivate

class PathChooserPrivate
{
public:
    PathChooserPrivate();

    QString expandedPath(const QString &path) const;

    QHBoxLayout *m_hLayout;
    FancyLineEdit *m_lineEdit;

    PathChooser::Kind m_acceptingKind;
    QString m_dialogTitleOverride;
    QString m_dialogFilter;
    QString m_initialBrowsePathOverride;
    QString m_baseDirectory;
    Environment m_environment;
    BinaryVersionToolTipEventFilter *m_binaryVersionToolTipEventFilter;
    QList<QAbstractButton *> m_buttons;
};

PathChooserPrivate::PathChooserPrivate() :
    m_hLayout(new QHBoxLayout),
    m_lineEdit(new FancyLineEdit),
    m_acceptingKind(PathChooser::ExistingDirectory),
    m_binaryVersionToolTipEventFilter(0)
{
}

QString PathChooserPrivate::expandedPath(const QString &input) const
{
    if (input.isEmpty())
        return input;
    const QString path = QDir::cleanPath(m_environment.expandVariables(input));
    if (path.isEmpty())
        return path;

    switch (m_acceptingKind) {
    case PathChooser::Command:
    case PathChooser::ExistingCommand: {
        const FileName expanded = m_environment.searchInPath(path, QStringList(m_baseDirectory));
        return expanded.isEmpty() ? path : expanded.toString();
    }
    case PathChooser::Any:
        break;
    case PathChooser::Directory:
    case PathChooser::ExistingDirectory:
    case PathChooser::File:
    case PathChooser::SaveFile:
        if (!m_baseDirectory.isEmpty() && QFileInfo(path).isRelative())
            return QFileInfo(m_baseDirectory + QLatin1Char('/') + path).absoluteFilePath();
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

    connect(d->m_lineEdit, &FancyLineEdit::customContextMenuRequested, this, &PathChooser::contextMenuRequested);
    connect(d->m_lineEdit, &FancyLineEdit::validReturnPressed, this, &PathChooser::returnPressed);
    connect(d->m_lineEdit, &QLineEdit::textChanged, this,
            [this] { emit rawPathChanged(rawPath()); });
    connect(d->m_lineEdit, &FancyLineEdit::validChanged, this, &PathChooser::validChanged);
    connect(d->m_lineEdit, &QLineEdit::editingFinished, this, &PathChooser::editingFinished);
    connect(d->m_lineEdit, &QLineEdit::textChanged, this, [this] { emit pathChanged(path()); });

    d->m_lineEdit->setMinimumWidth(120);
    d->m_lineEdit->setErrorColor(creatorTheme()->color(Theme::TextColorError));
    d->m_hLayout->addWidget(d->m_lineEdit);
    d->m_hLayout->setSizeConstraint(QLayout::SetMinimumSize);

    addButton(browseButtonLabel(), this, [this] { slotBrowse(); });

    setLayout(d->m_hLayout);
    setFocusProxy(d->m_lineEdit);
    setFocusPolicy(d->m_lineEdit->focusPolicy());
    setEnvironment(Environment::systemEnvironment());

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

void PathChooser::insertButton(int index, const QString &text, QObject *context, const std::function<void ()> &callback)
{
    auto button = new QPushButton;
    button->setText(text);
    connect(button, &QAbstractButton::clicked, context, callback);
    d->m_hLayout->insertWidget(index + 1/*line edit*/, button);
    d->m_buttons.insert(index, button);
}

QString PathChooser::browseButtonLabel()
{
    return HostOsInfo::isMacHost() ? tr("Choose...") : tr("Browse...");
}

QAbstractButton *PathChooser::buttonAtIndex(int index) const
{
    return d->m_buttons.at(index);
}

QString PathChooser::baseDirectory() const
{
    return d->m_baseDirectory;
}

void PathChooser::setBaseDirectory(const QString &directory)
{
    if (d->m_baseDirectory == directory)
        return;
    d->m_baseDirectory = directory;
    triggerChanged();
}

FileName PathChooser::baseFileName() const
{
    return FileName::fromString(d->m_baseDirectory);
}

void PathChooser::setBaseFileName(const FileName &base)
{
    d->m_baseDirectory = base.toString();
    triggerChanged();
}

void PathChooser::setEnvironment(const Environment &env)
{
    QString oldExpand = path();
    d->m_environment = env;
    if (path() != oldExpand) {
        triggerChanged();
        emit rawPathChanged(rawPath());
    }
}

QString PathChooser::rawPath() const
{
    return rawFileName().toString();
}

QString PathChooser::path() const
{
    return fileName().toString();
}

FileName PathChooser::rawFileName() const
{
    return FileName::fromString(QDir::fromNativeSeparators(d->m_lineEdit->text()));
}

FileName PathChooser::fileName() const
{
    return FileName::fromUserInput(d->expandedPath(rawFileName().toString()));
}

// FIXME: try to remove again
QString PathChooser::expandedDirectory(const QString &input, const Environment &env,
                                       const QString &baseDir)
{
    if (input.isEmpty())
        return input;
    const QString path = QDir::cleanPath(env.expandVariables(input));
    if (path.isEmpty())
        return path;
    if (!baseDir.isEmpty() && QFileInfo(path).isRelative())
        return QFileInfo(baseDir + QLatin1Char('/') + path).absoluteFilePath();
    return path;
}

void PathChooser::setPath(const QString &path)
{
    d->m_lineEdit->setText(QDir::toNativeSeparators(path));
}

void PathChooser::setFileName(const FileName &fn)
{
    d->m_lineEdit->setText(fn.toUserOutput());
}

void PathChooser::setErrorColor(const QColor &errorColor)
{
    d->m_lineEdit->setErrorColor(errorColor);
}

void PathChooser::setOkColor(const QColor &okColor)
{
    d->m_lineEdit->setOkColor(okColor);
}

bool PathChooser::isReadOnly() const
{
    return d->m_lineEdit->isReadOnly();
}

void PathChooser::setReadOnly(bool b)
{
    d->m_lineEdit->setReadOnly(b);
    foreach (QAbstractButton *button, d->m_buttons)
        button->setEnabled(!b);
}

void PathChooser::slotBrowse()
{
    emit beforeBrowsing();

    QString predefined = path();
    QFileInfo fi(predefined);

    if (!predefined.isEmpty() && !fi.isDir()) {
        predefined = fi.path();
        fi.setFile(predefined);
    }

    if ((predefined.isEmpty() || !fi.isDir())
            && !d->m_initialBrowsePathOverride.isNull()) {
        predefined = d->m_initialBrowsePathOverride;
        fi.setFile(predefined);
        if (!fi.isDir()) {
            predefined.clear();
            fi.setFile(QString());
        }
    }

    // Prompt for a file/dir
    QString newPath;
    switch (d->m_acceptingKind) {
    case PathChooser::Directory:
    case PathChooser::ExistingDirectory:
        newPath = QFileDialog::getExistingDirectory(this,
                makeDialogTitle(tr("Choose Directory")), predefined);
        break;
    case PathChooser::ExistingCommand:
    case PathChooser::Command:
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose Executable")), predefined,
                d->m_dialogFilter);
        newPath = appBundleExpandedPath(newPath);
        break;
    case PathChooser::File: // fall through
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose File")), predefined,
                d->m_dialogFilter);
        newPath = appBundleExpandedPath(newPath);
        break;
    case PathChooser::SaveFile:
        newPath = QFileDialog::getSaveFileName(this,
                makeDialogTitle(tr("Choose File")), predefined,
                d->m_dialogFilter);
        break;
    case PathChooser::Any: {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setWindowTitle(makeDialogTitle(tr("Choose File")));
        if (fi.exists())
            dialog.setDirectory(fi.absolutePath());
        // FIXME: fix QFileDialog so that it filters properly: lib*.a
        dialog.setNameFilter(d->m_dialogFilter);
        if (dialog.exec() == QDialog::Accepted) {
            // probably loop here until the *.framework dir match
            QStringList paths = dialog.selectedFiles();
            if (!paths.isEmpty())
                newPath = paths.at(0);
        }
        break;
        }

    default:
        break;
    }

    // Delete trailing slashes unless it is "/"|"\\", only
    if (!newPath.isEmpty()) {
        newPath = QDir::toNativeSeparators(newPath);
        if (newPath.size() > 1 && newPath.endsWith(QDir::separator()))
            newPath.truncate(newPath.size() - 1);
        setPath(newPath);
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

QColor PathChooser::errorColor() const
{
    return d->m_lineEdit->errorColor();
}

QColor PathChooser::okColor() const
{
    return d->m_lineEdit->okColor();
}

FancyLineEdit::ValidationFunction PathChooser::defaultValidationFunction() const
{
    return std::bind(&PathChooser::validatePath, this, std::placeholders::_1, std::placeholders::_2);
}

bool PathChooser::validatePath(FancyLineEdit *edit, QString *errorMessage) const
{
    const QString path = edit->text();
    QString expandedPath = d->expandedPath(path);

    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path must not be empty.");
        return false;
    }

    if (expandedPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path \"%1\" expanded to an empty string.").arg(QDir::toNativeSeparators(path));
        return false;
    }
    const QFileInfo fi(expandedPath);

    // Check if existing
    switch (d->m_acceptingKind) {
    case PathChooser::ExistingDirectory: // fall through
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        if (!fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" is not a directory.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::File: // fall through
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        if (!fi.isFile()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" is not a file.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::SaveFile:
        if (!fi.absoluteDir().exists()) {
            if (errorMessage)
                *errorMessage = tr("The directory \"%1\" does not exist.").arg(QDir::toNativeSeparators(fi.absolutePath()));
            return false;
        }
        if (fi.exists() && fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" is not a file.").arg(QDir::toNativeSeparators(fi.absolutePath()));
            return false;
        }
        break;
    case PathChooser::ExistingCommand:
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        if (!fi.isFile() || !fi.isExecutable()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" is not an executable file.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::Directory:
        if (fi.exists() && !fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path \"%1\" is not a directory.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::Command: // fall through
        if (fi.exists() && !fi.isExecutable()) {
            if (errorMessage)
                *errorMessage = tr("Cannot execute \"%1\".").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;

    default:
        ;
    }

    if (errorMessage)
        *errorMessage = tr("Full path: \"%1\"").arg(QDir::toNativeSeparators(expandedPath));
    return true;
}

void PathChooser::setValidationFunction(const FancyLineEdit::ValidationFunction &fn)
{
    d->m_lineEdit->setValidationFunction(fn);
}

QString PathChooser::label()
{
    return tr("Path:");
}

QString PathChooser::homePath()
{
    // Return 'users/<name>/Documents' on Windows, since Windows explorer
    // does not let people actually display the contents of their home
    // directory. Alternatively, create a QtCreator-specific directory?
    if (HostOsInfo::isWindowsHost())
        return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir::homePath();
}

void PathChooser::setExpectedKind(Kind expected)
{
    if (d->m_acceptingKind == expected)
        return;
    d->m_acceptingKind = expected;
    d->m_lineEdit->validate();
}

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
}

QString PathChooser::promptDialogFilter() const
{
    return d->m_dialogFilter;
}

void PathChooser::setInitialBrowsePathBackup(const QString &path)
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
        d->m_lineEdit->setObjectName(objectName() + QLatin1String("LineEdit"));
    return d->m_lineEdit;
}

QString PathChooser::toolVersion(const QString &binary, const QStringList &arguments)
{
    return BinaryVersionToolTipEventFilter::toolVersion(binary, arguments);
}

void PathChooser::installLineEditVersionToolTip(QLineEdit *le, const QStringList &arguments)
{
    BinaryVersionToolTipEventFilter *ef = new BinaryVersionToolTipEventFilter(le);
    ef->setArguments(arguments);
}

void PathChooser::setHistoryCompleter(const QString &historyKey, bool restoreLastItemFromHistory)
{
    d->m_lineEdit->setHistoryCompleter(historyKey, restoreLastItemFromHistory);
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
            d->m_binaryVersionToolTipEventFilter = 0;
        }
    } else {
        if (!d->m_binaryVersionToolTipEventFilter)
            d->m_binaryVersionToolTipEventFilter = new PathChooserBinaryVersionToolTipEventFilter(this);
        d->m_binaryVersionToolTipEventFilter->setArguments(arguments);
    }
}

} // namespace Utils
