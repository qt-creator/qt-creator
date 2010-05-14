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

#include "cdbsymbolpathlisteditor.h"
#include "cdboptions.h"

#include <utils/pathchooser.h>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtGui/QFileDialog>
#include <QtGui/QAction>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QMessageBox>

namespace Debugger {
namespace Internal {

CacheDirectoryDialog::CacheDirectoryDialog(QWidget *parent) :
    QDialog(parent), m_chooser(new Utils::PathChooser),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select Local Cache Folder"));
    setModal(true);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QFormLayout *formLayout = new QFormLayout;
    m_chooser->setExpectedKind(Utils::PathChooser::Directory);
    m_chooser->setMinimumWidth(400);
    formLayout->addRow(tr("Path:"), m_chooser);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_buttonBox);

    setLayout(mainLayout);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CacheDirectoryDialog::setPath(const QString &p)
{
    m_chooser->setPath(p);
}

QString CacheDirectoryDialog::path() const
{
    return m_chooser->path();
}

void CacheDirectoryDialog::accept()
{
    // Ensure path exists
    QString cache = path();
    if (cache.isEmpty())
        return;
    QFileInfo fi(cache);
    // Folder exists - all happy.
    if (fi.isDir()) {
        QDialog::accept();
        return;
    }
    // Does a file of the same name exist?
    if (fi.exists()) {
        QMessageBox::warning(this, tr("Already Exists"),
                             tr("A file named '%1' already exists.").arg(cache));
        return;
    }
    // Create
    QDir root(QDir::root());
    if (!root.mkpath(cache)) {
        QMessageBox::warning(this, tr("Cannot Create"),
                             tr("The folder '%1' could not be created.").arg(cache));
        return;
    }
    QDialog::accept();
}

CdbSymbolPathListEditor::CdbSymbolPathListEditor(QWidget *parent) :
    Utils::PathListEditor(parent)
{
    //! Add Microsoft Symbol server connection
    QAction *action = insertAction(lastAddActionIndex() + 1, tr("Symbol Server..."), this, SLOT(addSymbolServer()));
    action->setToolTip(tr("Adds the Microsoft symbol server providing symbols for operating system libraries."
                      "Requires specifying a local cache directory."));
}

QString CdbSymbolPathListEditor::promptCacheDirectory(QWidget *parent)
{
    CacheDirectoryDialog dialog(parent);
    dialog.setPath(QDir::tempPath() + QDir::separator() + QLatin1String("symbolcache"));
    if (dialog.exec() != QDialog::Accepted)
        return QString();
    return dialog.path();
}

void CdbSymbolPathListEditor::addSymbolServer()
{
    const QString cacheDir = promptCacheDirectory(this);
    if (!cacheDir.isEmpty())
        insertPathAtCursor(CdbOptions::symbolServerPath(cacheDir));
}

} // namespace Internal
} // namespace Debugger
