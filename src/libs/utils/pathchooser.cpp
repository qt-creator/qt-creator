/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "pathchooser.h"

#include "basevalidatinglineedit.h"
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

namespace Core {
namespace Utils {

#ifdef Q_OS_MAC
/*static*/ const char * const PathChooser::browseButtonLabel = "Choose...";
#else
/*static*/ const char * const PathChooser::browseButtonLabel = "Browse...";
#endif

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
struct PathChooserPrivate
{
    PathChooserPrivate(PathChooser *chooser);

    PathValidatingLineEdit *m_lineEdit;
    PathChooser::Kind m_acceptingKind;
    QString m_dialogTitleOverride;
    QString m_initialBrowsePathOverride;
};

PathChooserPrivate::PathChooserPrivate(PathChooser *chooser) :
    m_lineEdit(new PathValidatingLineEdit(chooser)),
    m_acceptingKind(PathChooser::Directory)
{
}

PathChooser::PathChooser(QWidget *parent) :
    QWidget(parent),
    m_d(new PathChooserPrivate(this))
{
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);

    connect(m_d->m_lineEdit, SIGNAL(validReturnPressed()), this, SIGNAL(returnPressed()));
    connect(m_d->m_lineEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_d->m_lineEdit, SIGNAL(validChanged()), this, SIGNAL(validChanged()));

    m_d->m_lineEdit->setMinimumWidth(200);
    hLayout->addWidget(m_d->m_lineEdit);
    hLayout->setSizeConstraint(QLayout::SetMinimumSize);

#ifdef Q_OS_MAC
    QPushButton *browseButton = new QPushButton;
#else
    QToolButton *browseButton = new QToolButton;
#endif
    browseButton->setText(tr(browseButtonLabel));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(slotBrowse()));

    hLayout->addWidget(browseButton);
    setLayout(hLayout);
    setFocusProxy(m_d->m_lineEdit);
}

PathChooser::~PathChooser()
{
    delete m_d;
}

QString PathChooser::path() const
{
    return m_d->m_lineEdit->text();
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
    QString dialogTitle;
    QString newPath;
    switch (m_d->m_acceptingKind) {
    case PathChooser::Directory:
        newPath = QFileDialog::getExistingDirectory(this,
                makeDialogTitle(tr("Choose a directory")), predefined);
        break;

    case PathChooser::File: // fall through
    case PathChooser::Command:
        newPath = QFileDialog::getOpenFileName(this,
                makeDialogTitle(tr("Choose a file")), predefined);
        break;

    default:
        ;
    }

    // Delete trailing slashes unless it is "/"|"\\", only
    if (!newPath.isEmpty()) {
        newPath = QDir::toNativeSeparators(newPath);
        if (newPath.size() > 1 && newPath.endsWith(QDir::separator()))
            newPath.truncate(newPath.size() - 1);
        setPath(newPath);
    }

    emit browsingFinished();
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
    if (path.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The path must not be empty.");
        return false;
    }

    const QFileInfo fi(path);
    const bool isDir = fi.isDir();

    // Check if existing
    switch (m_d->m_acceptingKind) {
    case PathChooser::Directory: // fall through
    case PathChooser::File:
        if (!fi.exists()) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' does not exist.").arg(path);
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
        if (!isDir) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a directory.").arg(path);
            return false;
        }
        break;

    case PathChooser::File:
        if (isDir) {
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a file.").arg(path);
            return false;
        }
        break;

    case PathChooser::Command:
        // TODO do proper command validation
        // i.e. search $PATH for a matching file
        break;

    default:
        ;
    }

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

void PathChooser::setPromptDialogTitle(const QString &title)
{
    m_d->m_dialogTitleOverride = title;
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

} // namespace Utils
} // namespace Core
