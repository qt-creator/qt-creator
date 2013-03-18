/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pathchooser.h"

#include "fancylineedit.h"
#include "basevalidatinglineedit.h"
#include "environment.h"
#include "qtcassert.h"

#include "synchronousprocess.h"
#include "hostosinfo.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QPushButton>

/*!
    \class Utils::PathChooser

    \brief A control that let's the user choose a path, consisting of a QLineEdit and
    a "Browse" button.

    Has some validation logic for embedding into QWizardPage.
*/

const char * const Utils::PathChooser::browseButtonLabel =
#ifdef Q_OS_MAC
                   QT_TRANSLATE_NOOP("Utils::PathChooser", "Choose...");
#else
                   QT_TRANSLATE_NOOP("Utils::PathChooser", "Browse...");
#endif

namespace Utils {

// ------------------ PathValidatingLineEdit

class PathValidatingLineEdit : public BaseValidatingLineEdit
{
public:
    explicit PathValidatingLineEdit(PathChooser *chooser, QWidget *parent = 0);

protected:
    virtual bool validate(const QString &value, QString *errorMessage) const;

private:
    PathChooser *m_chooser;
};

PathValidatingLineEdit::PathValidatingLineEdit(PathChooser *chooser, QWidget *parent) :
    BaseValidatingLineEdit(parent),
    m_chooser(chooser)
{
    QTC_ASSERT(chooser, return);
}

bool PathValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return m_chooser->validatePath(value, errorMessage);
}

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
    QProcess proc;
    proc.start(binary, arguments);
    if (!proc.waitForStarted())
        return QString();
    if (!proc.waitForFinished()) {
        Utils::SynchronousProcess::stopProcess(proc);
        return QString();
    }
    return QString::fromLocal8Bit(QByteArray(proc.readAllStandardOutput()
        + proc.readAllStandardError()));
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
    PathChooserPrivate(PathChooser *chooser);

    QString expandedPath(const QString &path) const;

    QHBoxLayout *m_hLayout;
    PathValidatingLineEdit *m_lineEdit;

    PathChooser::Kind m_acceptingKind;
    QString m_dialogTitleOverride;
    QString m_dialogFilter;
    QString m_initialBrowsePathOverride;
    QString m_baseDirectory;
    Environment m_environment;
    BinaryVersionToolTipEventFilter *m_binaryVersionToolTipEventFilter;
    QList<QAbstractButton *> m_buttons;
};

PathChooserPrivate::PathChooserPrivate(PathChooser *chooser) :
    m_hLayout(new QHBoxLayout),
    m_lineEdit(new PathValidatingLineEdit(chooser)),
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
        const QString expanded = m_environment.searchInPath(path, QStringList(m_baseDirectory));
        return expanded.isEmpty() ? path : expanded;
    }
    case PathChooser::Any:
        break;
    case PathChooser::Directory:
    case PathChooser::ExistingDirectory:
    case PathChooser::File:
        if (!m_baseDirectory.isEmpty() && QFileInfo(path).isRelative())
            return QFileInfo(m_baseDirectory + QLatin1Char('/') + path).absoluteFilePath();
        break;
    }
    return path;
}

PathChooser::PathChooser(QWidget *parent) :
    QWidget(parent),
    d(new PathChooserPrivate(this))
{
    d->m_hLayout->setContentsMargins(0, 0, 0, 0);

    connect(d->m_lineEdit, SIGNAL(validReturnPressed()), this, SIGNAL(returnPressed()));
    connect(d->m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed(QString)));
    connect(d->m_lineEdit, SIGNAL(validChanged()), this, SIGNAL(validChanged()));
    connect(d->m_lineEdit, SIGNAL(validChanged(bool)), this, SIGNAL(validChanged(bool)));
    connect(d->m_lineEdit, SIGNAL(editingFinished()), this, SIGNAL(editingFinished()));

    d->m_lineEdit->setMinimumWidth(120);
    d->m_hLayout->addWidget(d->m_lineEdit);
    d->m_hLayout->setSizeConstraint(QLayout::SetMinimumSize);

    addButton(tr(browseButtonLabel), this, SLOT(slotBrowse()));

    setLayout(d->m_hLayout);
    setFocusProxy(d->m_lineEdit);
    setFocusPolicy(d->m_lineEdit->focusPolicy());
    setEnvironment(Environment::systemEnvironment());
}

PathChooser::~PathChooser()
{
    delete d;
}

void PathChooser::addButton(const QString &text, QObject *receiver, const char *slotFunc)
{
    insertButton(d->m_buttons.count(), text, receiver, slotFunc);
}

void PathChooser::insertButton(int index, const QString &text, QObject *receiver, const char *slotFunc)
{
    QPushButton *button = new QPushButton;
    button->setText(text);
    connect(button, SIGNAL(clicked()), receiver, slotFunc);
    d->m_hLayout->insertWidget(index + 1/*line edit*/, button);
    d->m_buttons.insert(index, button);
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
    d->m_baseDirectory = directory;
}

FileName PathChooser::baseFileName() const
{
    return Utils::FileName::fromString(d->m_baseDirectory);
}

void PathChooser::setBaseFileName(const FileName &base)
{
    d->m_baseDirectory = base.toString();
}

void PathChooser::setEnvironment(const Utils::Environment &env)
{
    QString oldExpand = path();
    d->m_environment = env;
    if (path() != oldExpand)
        emit changed(rawPath());
}

QString PathChooser::path() const
{
    return d->expandedPath(QDir::fromNativeSeparators(d->m_lineEdit->text()));
}

QString PathChooser::rawPath() const
{
    return QDir::fromNativeSeparators(d->m_lineEdit->text());
}

FileName PathChooser::fileName() const
{
    return Utils::FileName::fromString(path());
}

void PathChooser::setPath(const QString &path)
{
    d->m_lineEdit->setText(QDir::toNativeSeparators(path));
}

void PathChooser::setFileName(const Utils::FileName &fn)
{
    d->m_lineEdit->setText(fn.toUserOutput());
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
    if ((predefined.isEmpty() || !QFileInfo(predefined).isDir())
            && !d->m_initialBrowsePathOverride.isNull()) {
        predefined = d->m_initialBrowsePathOverride;
        if (!QFileInfo(predefined).isDir())
            predefined.clear();
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
        break;
    case PathChooser::File: // fall through
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose File")), predefined,
                d->m_dialogFilter);
        break;
    case PathChooser::Any: {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setWindowTitle(makeDialogTitle(tr("Choose File")));
        QFileInfo fi(predefined);
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
    d->m_lineEdit->triggerChanged();
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
    d->m_lineEdit->triggerChanged();
}

bool PathChooser::validatePath(const QString &path, QString *errorMessage)
{
    QString expandedPath = d->expandedPath(path);

    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path must not be empty.");
        return false;
    }

    if (expandedPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path '%1' expanded to an empty string.").arg(QDir::toNativeSeparators(path));
        return false;
    }
    const QFileInfo fi(expandedPath);

    // Check if existing
    switch (d->m_acceptingKind) {
    case PathChooser::ExistingDirectory: // fall through
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        if (!fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a directory.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::File: // fall through
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::ExistingCommand:
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        if (!fi.isExecutable()) {
            if (errorMessage)
                *errorMessage = tr("Cannot execute '%1'.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::Directory:
        if (fi.exists() && !fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a directory.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;
    case PathChooser::Command: // fall through
        if (fi.exists() && !fi.isExecutable()) {
            if (errorMessage)
                *errorMessage = tr("Cannot execute '%1'.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;

    default:
        ;
    }

    // Check expected kind
    switch (d->m_acceptingKind) {
    case PathChooser::ExistingDirectory:
        if (!fi.isDir()) {
            if (errorMessage)
                *errorMessage = tr("The path <b>%1</b> is not a directory.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;

    case PathChooser::File:
        if (!fi.isFile()) {
            if (errorMessage)
                *errorMessage = tr("The path <b>%1</b> is not a file.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;

    case PathChooser::ExistingCommand:
        if (!fi.isFile() || !fi.isExecutable()) {
            if (errorMessage)
                *errorMessage = tr("The path <b>%1</b> is not an executable file.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }

    case PathChooser::Command:
        break;

    case PathChooser::Any:
        break;

    default:
        ;
    }
    if (errorMessage)
        *errorMessage = tr("Full path: <b>%1</b>").arg(QDir::toNativeSeparators(expandedPath));
    return true;
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
        return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    return QDir::homePath();
}

void PathChooser::setExpectedKind(Kind expected)
{
    if (d->m_acceptingKind == expected)
        return;
    d->m_acceptingKind = expected;
    d->m_lineEdit->triggerChanged();
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
