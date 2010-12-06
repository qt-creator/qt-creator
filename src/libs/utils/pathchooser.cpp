/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "pathchooser.h"

#include "basevalidatinglineedit.h"
#include "environment.h"
#include "qtcassert.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

#include <QtGui/QDesktopServices>
#include <QtGui/QFileDialog>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QToolButton>
#include <QtGui/QPushButton>

/*static*/ const char * const Utils::PathChooser::browseButtonLabel =
#ifdef Q_WS_MAC
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
};

PathChooserPrivate::PathChooserPrivate(PathChooser *chooser) :
    m_hLayout(new QHBoxLayout),
    m_lineEdit(new PathValidatingLineEdit(chooser)),
    m_acceptingKind(PathChooser::Directory)
{
}

QString PathChooserPrivate::expandedPath(const QString &input) const
{
    if (input.isEmpty())
        return input;
    const QString path = QDir::fromNativeSeparators(m_environment.expandVariables(input));
    if (path.isEmpty())
        return path;

    switch (m_acceptingKind) {
    case PathChooser::Command:
    case PathChooser::ExistingCommand: {
        const QString expanded = m_environment.searchInPath(path, QStringList(m_baseDirectory));
        return expanded.isEmpty() && m_acceptingKind == PathChooser::Command ? path : expanded;
    }
    case PathChooser::Any:
        break;
    case PathChooser::Directory:
    case PathChooser::File:
        if (!m_baseDirectory.isEmpty() && QFileInfo(path).isRelative())
            return QFileInfo(m_baseDirectory + QLatin1Char('/') + path).absoluteFilePath();
        break;
    }
    return path;
}

PathChooser::PathChooser(QWidget *parent) :
    QWidget(parent),
    m_d(new PathChooserPrivate(this))
{
    m_d->m_hLayout->setContentsMargins(0, 0, 0, 0);

    connect(m_d->m_lineEdit, SIGNAL(validReturnPressed()), this, SIGNAL(returnPressed()));
    connect(m_d->m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed(QString)));
    connect(m_d->m_lineEdit, SIGNAL(validChanged()), this, SIGNAL(validChanged()));
    connect(m_d->m_lineEdit, SIGNAL(validChanged(bool)), this, SIGNAL(validChanged(bool)));
    connect(m_d->m_lineEdit, SIGNAL(editingFinished()), this, SIGNAL(editingFinished()));

    m_d->m_lineEdit->setMinimumWidth(200);
    m_d->m_hLayout->addWidget(m_d->m_lineEdit);
    m_d->m_hLayout->setSizeConstraint(QLayout::SetMinimumSize);

    addButton(tr(browseButtonLabel), this, SLOT(slotBrowse()));

    setLayout(m_d->m_hLayout);
    setFocusProxy(m_d->m_lineEdit);
    setEnvironment(Environment::systemEnvironment());
}

PathChooser::~PathChooser()
{
    delete m_d;
}

void PathChooser::addButton(const QString &text, QObject *receiver, const char *slotFunc)
{
#ifdef Q_WS_MAC
    QPushButton *button = new QPushButton;
#else
    QToolButton *button = new QToolButton;
#endif
    button->setText(text);
    connect(button, SIGNAL(clicked()), receiver, slotFunc);
    m_d->m_hLayout->addWidget(button);
}

QAbstractButton *PathChooser::buttonAtIndex(int index) const
{
    return findChildren<QAbstractButton*>().at(index);
}

QString PathChooser::baseDirectory() const
{
    return m_d->m_baseDirectory;
}

void PathChooser::setBaseDirectory(const QString &directory)
{
    m_d->m_baseDirectory = directory;
}

void PathChooser::setEnvironment(const Utils::Environment &env)
{
    QString oldExpand = path();
    m_d->m_environment = env;
    if (path() != oldExpand)
        emit changed(rawPath());
}

QString PathChooser::path() const
{
    return m_d->expandedPath(QDir::fromNativeSeparators(m_d->m_lineEdit->text()));
}

QString PathChooser::rawPath() const
{
    return QDir::fromNativeSeparators(m_d->m_lineEdit->text());
}

void PathChooser::setPath(const QString &path)
{
    m_d->m_lineEdit->setText(QDir::toNativeSeparators(path));
}

void PathChooser::slotBrowse()
{
    emit beforeBrowsing();

    QString predefined = path();
    if ((predefined.isEmpty() || !QFileInfo(predefined).isDir())
            && !m_d->m_initialBrowsePathOverride.isNull()) {
        predefined = m_d->m_initialBrowsePathOverride;
        if (!QFileInfo(predefined).isDir())
            predefined.clear();
    }

    // Prompt for a file/dir
    QString newPath;
    switch (m_d->m_acceptingKind) {
    case PathChooser::Directory:
        newPath = QFileDialog::getExistingDirectory(this,
                makeDialogTitle(tr("Choose Directory")), predefined);
        break;
    case PathChooser::ExistingCommand:
    case PathChooser::Command:
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose Executable")), predefined,
                m_d->m_dialogFilter);
        break;
    case PathChooser::File: // fall through
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose File")), predefined,
                m_d->m_dialogFilter);
        break;
    case PathChooser::Any: {
        QFileDialog dialog(this);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setWindowTitle(makeDialogTitle(tr("Choose File")));
        QFileInfo fi(predefined);
        if (fi.exists())
            dialog.setDirectory(fi.absolutePath());
        // FIXME: fix QFileDialog so that it filters properly: lib*.a
        dialog.setNameFilter(m_d->m_dialogFilter);
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
    m_d->m_lineEdit->triggerChanged();
}

bool PathChooser::isValid() const
{
    return m_d->m_lineEdit->isValid();
}

QString PathChooser::errorMessage() const
{
    return m_d->m_lineEdit->errorMessage();
}

bool PathChooser::validatePath(const QString &path, QString *errorMessage)
{
    QString expandedPath = m_d->expandedPath(path);

    QString displayPath = expandedPath;
    if (expandedPath.isEmpty())
        //: Selected path is not valid:
        displayPath = tr("<not valid>");

    if (expandedPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path must not be empty.");
        return false;
    }

    const QFileInfo fi(expandedPath);

    // Check if existing
    switch (m_d->m_acceptingKind) {
    case PathChooser::Directory: // fall through
    case PathChooser::File: // fall through
    case PathChooser::ExistingCommand:
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' does not exist.").arg(QDir::toNativeSeparators(expandedPath));
            return false;
        }
        break;

    case PathChooser::Command: // fall through
    default:
        ;
    }

    // Check expected kind
    switch (m_d->m_acceptingKind) {
    case PathChooser::Directory:
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
                *errorMessage = tr("The path <b>%1</b> is not a executable file.").arg(QDir::toNativeSeparators(expandedPath));
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
#ifdef Q_OS_WIN
    // Return 'users/<name>/Documents' on Windows, since Windows explorer
    // does not let people actually display the contents of their home
    // directory. Alternatively, create a QtCreator-specific directory?
    return QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
    return QDir::homePath();
#endif
}

void PathChooser::setExpectedKind(Kind expected)
{
    m_d->m_acceptingKind = expected;
}

PathChooser::Kind PathChooser::expectedKind() const
{
    return m_d->m_acceptingKind;
}

void PathChooser::setPromptDialogTitle(const QString &title)
{
    m_d->m_dialogTitleOverride = title;
}

QString PathChooser::promptDialogTitle() const
{
    return m_d->m_dialogTitleOverride;
}

void PathChooser::setPromptDialogFilter(const QString &filter)
{
    m_d->m_dialogFilter = filter;
}

QString PathChooser::promptDialogFilter() const
{
    return m_d->m_dialogFilter;
}

void PathChooser::setInitialBrowsePathBackup(const QString &path)
{
    m_d->m_initialBrowsePathOverride = path;
}

QString PathChooser::makeDialogTitle(const QString &title)
{
    if (m_d->m_dialogTitleOverride.isNull())
        return title;
    else
        return m_d->m_dialogTitleOverride;
}

QLineEdit *PathChooser::lineEdit() const
{
    // HACK: Make it work with HistoryCompleter.
    if (m_d->m_lineEdit->objectName().isEmpty())
        m_d->m_lineEdit->setObjectName(objectName() + QLatin1String("LineEdit"));
    return m_d->m_lineEdit;
}

} // namespace Utils
