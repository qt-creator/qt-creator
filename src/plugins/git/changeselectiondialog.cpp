/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "changeselectiondialog.h"

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Git {
namespace Internal {

ChangeSelectionDialog::ChangeSelectionDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    connect(m_ui.repositoryButton, SIGNAL(clicked()), this, SLOT(selectWorkingDirectory()));
    setWindowTitle(tr("Select a Git Commit"));
}

QString ChangeSelectionDialog::change() const
{
    return m_ui.changeNumberEdit->text();
}

QString ChangeSelectionDialog::repository() const
{
    return m_ui.repositoryEdit->text();
}

void ChangeSelectionDialog::setRepository(const QString &s)
{
    m_ui.repositoryEdit->setText(QDir::toNativeSeparators(s));
    m_ui.changeNumberEdit->setFocus(Qt::ActiveWindowFocusReason);
    m_ui.changeNumberEdit->setText(QLatin1String("HEAD"));
    m_ui.changeNumberEdit->selectAll();
}

void ChangeSelectionDialog::selectWorkingDirectory()
{
    static QString location;
    location = QFileDialog::getExistingDirectory(this,
                                      tr("Select Git Repository"),
                                      location);
    if (location.isEmpty())
        return;

    // Verify that the location is a repository
    // We are polite, we also allow to specify a directory, which is not
    // the head directory of the repository.
    QDir repository(location);
    do {
        if (repository.entryList(QDir::AllDirs|QDir::Hidden).contains(QLatin1String(".git"))) {
            m_ui.repositoryEdit->setText(repository.absolutePath());
            return;
        }
    } while (repository.cdUp());

    // Did not find a repo
    QMessageBox::critical(this, tr("Error"),
                          tr("Selected directory is not a Git repository"));

}

}
}
