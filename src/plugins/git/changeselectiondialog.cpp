/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "changeselectiondialog.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <QFileDialog>
#include <QMessageBox>

namespace Git {
namespace Internal {

ChangeSelectionDialog::ChangeSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(m_ui.workingDirectoryButton, SIGNAL(clicked()), this, SLOT(selectWorkingDirectory()));
    setWindowTitle(tr("Select a Git Commit"));
}

QString ChangeSelectionDialog::change() const
{
    return m_ui.changeNumberEdit->text();
}

QString ChangeSelectionDialog::workingDirectory() const
{
    return m_ui.workingDirectoryEdit->text();
}

void ChangeSelectionDialog::setWorkingDirectory(const QString &s)
{
    m_ui.workingDirectoryEdit->setText(QDir::toNativeSeparators(s));
    m_ui.changeNumberEdit->setFocus(Qt::ActiveWindowFocusReason);
    m_ui.changeNumberEdit->setText(QLatin1String("HEAD"));
    m_ui.changeNumberEdit->selectAll();
}

void ChangeSelectionDialog::selectWorkingDirectory()
{
    QString location = QFileDialog::getExistingDirectory(this, tr("Select Working Directory"),
                                                         m_ui.workingDirectoryEdit->text());
    if (location.isEmpty())
        return;

    // Verify that the location is a repository
    // We allow specifying a directory, which is not the head directory of the repository.
    // This is useful for git show commit:./file
    QString topLevel = GitPlugin::instance()->gitClient()->findRepositoryForDirectory(location);
    if (!topLevel.isEmpty())
        m_ui.workingDirectoryEdit->setText(location);
    else // Did not find a repo
        QMessageBox::critical(this, tr("Error"),
                              tr("Selected directory is not a Git repository"));
}

}
}
