/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "pathchooser.h"
#include "basevalidatinglineedit.h"

#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopServices>

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QDebug>

namespace Core {
namespace Utils {

#ifdef Q_OS_OSX
/*static*/ const char * const PathChooser::browseButtonLabel = "Choose...";
#else
/*static*/ const char * const PathChooser::browseButtonLabel = "Browse...";
#endif

// ------------------ PathValidatingLineEdit
class PathValidatingLineEdit : public BaseValidatingLineEdit {
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
    Q_ASSERT(chooser != NULL);
}

bool PathValidatingLineEdit::validate(const QString &value, QString *errorMessage) const
{
    return m_chooser->validatePath(value, errorMessage);
}

// ------------------ PathChooserPrivate
struct PathChooserPrivate {
    PathChooserPrivate(PathChooser *chooser);

    PathValidatingLineEdit *m_lineEdit;
    PathChooser::Kind m_acceptingKind;
    QString m_dialogTitleOverride;
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

    m_d->m_lineEdit->setMinimumWidth(260);
    hLayout->addWidget(m_d->m_lineEdit);
    hLayout->setSizeConstraint(QLayout::SetMinimumSize);

    QToolButton *browseButton = new QToolButton;
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
    const QString defaultPath = path.isEmpty() ? homePath() : path;
    m_d->m_lineEdit->setText(QDir::toNativeSeparators(defaultPath));
}

void PathChooser::slotBrowse()
{
    QString predefined = path();
    if (!predefined.isEmpty() && !QFileInfo(predefined).isDir())
        predefined.clear();

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

    // TODO make cross-platform
    // Delete trailing slashes unless it is "/", only
    if (!newPath .isEmpty()) {
        if (newPath .size() > 1 && newPath .endsWith(QDir::separator()))
            newPath .truncate(newPath .size() - 1);
        setPath(newPath);
    }
}

bool PathChooser::isValid() const
{
    return m_d->m_lineEdit->isValid();
}

QString PathChooser::errorMessage() const
{
    return  m_d->m_lineEdit->errorMessage();
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
        if (!isDir)
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a directory.").arg(path);
            return false;
        break;

    case PathChooser::File:
        if (isDir)
            if (errorMessage)
                *errorMessage = tr("The path '%1' is not a file.").arg(path);
            return false;
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

QString PathChooser::makeDialogTitle(const QString &title)
{
    if (m_d->m_dialogTitleOverride.isNull())
        return title;
    else
        return m_d->m_dialogTitleOverride;
}

} // namespace Utils
} // namespace Core
